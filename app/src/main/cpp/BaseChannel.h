//
// Created by Administrator on 2018/9/7.
//

#ifndef PLAYER_BASECHANNEL_H
#define PLAYER_BASECHANNEL_H


#include "safe_queue.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

class BaseChannel {
public:
    BaseChannel(int id, AVCodecContext *avCodecContext, AVRational time_base) : id(id), avCodecContext(avCodecContext),time_base(time_base) {
        packets.setReleaseCallback(BaseChannel::releaseAvPacket);
        frames.setReleaseCallback(BaseChannel::releaseAvFrame);
    }

    //virtual
    virtual ~BaseChannel() {
        packets.clear();
        frames.clear();
    }

    /**
     * 释放 AVPacket
     * @param packet
     */
    static void releaseAvPacket(AVPacket** packet) {
        if (packet) {
            av_packet_free(packet);
            //为什么用指针的指针？
            // 指针的指针能够修改传递进来的指针的指向
            *packet = 0;
        }
    }

    static void releaseAvFrame(AVFrame** frame){
        if (frame) {
            av_frame_free(frame);
            //为什么用指针的指针？
            // 指针的指针能够修改传递进来的指针的指向
            *frame = 0;
        }
    }

    //纯虚方法 相当于 抽象方法
    virtual void play() = 0;
    virtual void stop() = 0;

    int id;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    bool isPlaying;
    AVCodecContext *avCodecContext;
    AVRational time_base;
};

#endif //PLAYER_BASECHANNEL_H
