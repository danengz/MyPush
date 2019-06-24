//
// Created by 张宇 on 2019-06-21.
//

#ifndef MYPUSH_AUDIOCHANNEL_H
#define MYPUSH_AUDIOCHANNEL_H


#include <faac.h>
#include <jni.h>
#include <pty.h>
#include "librtmp/rtmp.h"

class AudioChannel {
    typedef void (*AudioCallback)(RTMPPacket *packet);
public:

    void encodeData(int8_t *data);
    void setAudioEncInfo(int samplesInHZ, int channels);

    jint getInputSamples();
    ~AudioChannel();

    void setAudioCallback(AudioCallback audioCallback);
private:
    AudioCallback audioCallback;
    int mChannels;
    faacEncHandle audioCodec;
    u_long inputSamples;
    u_long maxOutputBytes;
    u_char *buffer = 0;
};


#endif //MYPUSH_AUDIOCHANNEL_H
