//
// Created by Administrator on 2018/9/5.
//

#ifndef PLAYER_AUDIOCHANNEL_H
#define PLAYER_AUDIOCHANNEL_H


#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "BaseChannel.h"
extern "C"{
#include <libswresample/swresample.h>
};

class AudioChannel: public BaseChannel {
public:
    AudioChannel(int id,AVCodecContext *avCodecContext, AVRational time_base);

    ~AudioChannel();

    void play();

    void decode();

    void _play();

    void stop();

    int getPCM();

public:
    //声道数
    int out_channels;
    //采样位 16bit
    int out_sampleSize;
    //采样率 Hz
    int out_sample_rate;
    //转换后的数据
    uint8_t *data = 0;
    //时间戳 用于同步
    double clock;
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;


    /**
     * OpenSL ES
     */
    // 引擎与引擎接口
    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerInterface = 0;

    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface =0;

    //转码器
    SwrContext *swrContext;
};


#endif //PLAYER_AUDIOCHANNEL_H
