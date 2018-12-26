//
// Created by Administrator on 2018/9/5.
//

#include "VideoChannel.h"

void *decode_task(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->decode();
    return 0;
}

void *render_task(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->render();
    return 0;
}

/**
 * 丢已经解码的图片
 * @param q
 */
void dropAvFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAvFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int id, AVCodecContext *avCodecContext, AVRational time_base,
                           int fps) : BaseChannel(id,
                                                  avCodecContext, time_base) {
    this->fps = fps;
    //用于同步操作队列
    frames.setSyncHandle(dropAvFrame);
}

VideoChannel::~VideoChannel() {

}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;
}


void VideoChannel::play() {
    isPlaying = 1;
    //设置为工作状态
    frames.setWork(1);
    packets.setWork(1);
    //1、解码
    pthread_create(&pid_decode, 0, decode_task, this);
    //2、播放
    pthread_create(&pid_render, 0, render_task, this);
}

//解码
void VideoChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        //取出一个数据包
        int ret = packets.pop(packet);
        if (!isPlaying) {
            break;
        }
        //取出失败
        if (!ret) {
            continue;
        }
        //把包丢给解码器
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(&packet);
        //重试
        if (ret != 0) {
            break;
        }
        //代表了一个图像 (将这个图像先输出来)
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取 解码后的数据包 AVFrame
        ret = avcodec_receive_frame(avCodecContext, frame);
        //需要更多的数据才能够进行解码
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        //再开一个线程 来播放 (流畅度)
        frames.push(frame);
    }
    releaseAvPacket(&packet);
}

//播放
void VideoChannel::render() {
    //目标： RGBA
    swsContext = sws_getContext(
            avCodecContext->width, avCodecContext->height, avCodecContext->pix_fmt,
            avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);
    AVFrame *frame = 0;
    //指针数组
    uint8_t *dst_data[4];
    int dst_linesize[4];
    av_image_alloc(dst_data, dst_linesize,
                   avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA, 1);

    //fps 单位:秒
    int frame_delays = 1.0 / fps;

    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        //src_linesize: 表示每一行存放的 字节长度
        sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                  frame->linesize, 0,
                  avCodecContext->height,
                  dst_data,
                  dst_linesize);

        int extra_delay = frame->repeat_pict / (2 * fps);
        double delays = frame_delays + extra_delay;
        double clock = frame->best_effort_timestamp * av_q2d(time_base);
        if (!audioChannel) {
            //休眠, 按fps来进行播放
            av_usleep(delays * 1000000);
        } else {
            if (clock == 0) {
                //开始播放
                //休眠, 按fps来进行播放
                av_usleep(delays * 1000000);
            } else {

                double diff = clock - audioChannel->clock;
                if (diff > 0) {
                    LOGE("视频快了:%lf", diff);
                    av_usleep((delays + diff) * 1000000);
                } else if (diff < 0) {
                    LOGE("音频快了:%lf", diff);
                    //丢包
                    //本张图片先丢
                    if (fabs(diff) >= 0.05) {
                        releaseAvFrame(&frame);
                        frames.sync();
                        continue;
                    } else {
                        //do nothing
                    }
                }
            }
        }

        //回调出去进行播放
        callback(dst_data[0], dst_linesize[0], avCodecContext->width, avCodecContext->height);
        releaseAvFrame(&frame);
    }
    av_freep(&dst_data[0]);
    releaseAvFrame(&frame);
    isPlaying = 0;
    sws_freeContext(swsContext);
    swsContext=0;
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback callback) {
    this->callback = callback;
}

void VideoChannel::stop() {
    LOGE("VideoChannel::stop Now");
    isPlaying = 0;
    //设置为工作状态
    frames.setWork(0);
    packets.setWork(0);
    //等待线程完成
    //1、解码
    pthread_join(pid_decode, 0);
    //2、播放
    pthread_join(pid_render, 0);
}