//
// Created by 张宇 on 2019-06-21.
//

#include <pty.h>
#include <jni.h>
#include <cstring>
#include "AudioChannel.h"
#include "faac.h"
#include "librtmp/rtmp.h"
#include "macro.h"


//不断调用
void AudioChannel::encodeData(int8_t *data) {
    int bytelen= faacEncEncode(audioCodec, reinterpret_cast<int32_t *>(data), inputSamples, buffer, maxOutputBytes);
    if (bytelen > 0) {
        RTMPPacket *packet = new RTMPPacket;
        int bodySize = 2 + bytelen;
        RTMPPacket_Alloc(packet, bodySize);
        packet->m_body[0] = 0xAF;
        if (mChannels == 1) {
            packet->m_body[0] = 0xAE;
        }
        packet->m_body[1] = 0x01;

        memcpy(&packet->m_body[2], buffer, bytelen);

        packet->m_hasAbsTimestamp = 0;
        packet->m_nBodySize = bodySize;
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nChannel = 0x11;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        audioCallback(packet);
    }

}
void AudioChannel::setAudioCallback(AudioChannel::AudioCallback audioCallback) {
    this->audioCallback = audioCallback;
}
// 初始初始化
void AudioChannel::setAudioEncInfo(int samplesInHZ, int channels){
    audioCodec=faacEncOpen(samplesInHZ, channels, &inputSamples, &maxOutputBytes);
    faacEncConfigurationPtr config=faacEncGetCurrentConfiguration(audioCodec);
    config->mpegVersion = MPEG4;
    config->aacObjectType = LOW;
    config->inputFormat = FAAC_INPUT_16BIT;
    config->outputFormat = 0;
    faacEncSetConfiguration(audioCodec, config);
    buffer = new u_char[maxOutputBytes];
}

int AudioChannel::getInputSamples() {
    return inputSamples;
}

AudioChannel::~AudioChannel() {
    DELETE(buffer);
    //释放编码器
    if (audioCodec) {
        faacEncClose(audioCodec);
        audioCodec = 0;
    }
}
