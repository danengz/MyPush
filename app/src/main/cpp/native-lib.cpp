#include <jni.h>
#include <string>
#include "x264.h"
#include "librtmp/rtmp.h"
#include "VideoChannel.h"
#include "pthread.h"
#include "macro.h"
#include "safe_queue.h"

VideoChannel *videoChannel;
int isStart = 0;//为了防止用户重复点击开始直播，导致重新初始化
pthread_t pid;
uint32_t start_time;
int readyPushing = 0;
//队列
SafeQueue<RTMPPacket *> packets;

void callback(RTMPPacket *packet) {

    if (packet) {

        //设置时间戳
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
//        加入队列
        packets.put(packet);
    }
}


/**
 * 释放packet
 * @param packet
 */
void releasePackets(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        delete packet;
        packet = 0;
    }

}

/**
 * 开始推流
 * @param args
 * @return
 */
void *start(void *args) {

    char *url = static_cast<char *>(args);
    RTMP *rtmp = 0;
    rtmp = RTMP_Alloc();
    if (!rtmp) {
        LOGE("alloc rtmp失败");
        return NULL;
    }

    RTMP_Init(rtmp);
    int ret = RTMP_SetupURL(rtmp, url);
    if (!ret) {
        LOGE("设置地址失败:%s", url);
        return NULL;
    }

    rtmp->Link.timeout = 5;
    RTMP_EnableWrite(rtmp);
    ret = RTMP_Connect(rtmp, 0);
    if (!ret) {
        LOGE("连接服务器:%s", url);
        return NULL;
    }

    ret = RTMP_ConnectStream(rtmp, 0);
    if (!ret) {
        LOGE("连接流:%s", url);
        return NULL;
    }
    start_time= RTMP_GetTime();
    //表示可以开始推流了
    readyPushing = 1;
    packets.setWork(1);
    RTMPPacket *packet = 0;
    while (readyPushing) {
//        队列取数据  pakets
        packets.get(packet);
        LOGE("取出一帧数据");
        if (!readyPushing) {
            break;
        }
        if (!packet) {
            continue;
        }
        packet->m_nInfoField2 = rtmp->m_stream_id;
        ret = RTMP_SendPacket(rtmp, packet, 1);

//        packet 释放
        releasePackets(packet);
    }

    isStart = 0;
    readyPushing = 0;
    packets.setWork(0);
    packets.clear();
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete (url);
    return  0;

}


extern "C"
JNIEXPORT void JNICALL
Java_com_yu_mypush_LivePusher_native_1init(JNIEnv *env, jobject instance) {

    videoChannel = new VideoChannel;
    videoChannel->setVideoCallback(callback);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_yu_mypush_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject instance, jint width, jint height,
                                                      jint fps, jint bitrate) {

    if (!videoChannel) {
        return;
    }
    videoChannel->setVideoEncInfo(width, height, fps, bitrate);

}


extern "C"
JNIEXPORT void JNICALL
Java_com_yu_mypush_LivePusher_native_1start(JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);

    if (isStart) {
        return;
    }
    isStart = 1;

    // path会回收
    char *url = new char[strlen(path) + 1];
    strcpy(url, path);

    //start类似java线程中的run方法，url是start的参数
    pthread_create(&pid, 0, start, url);

    env->ReleaseStringUTFChars(path_, path);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_yu_mypush_LivePusher_native_1pushVideo(JNIEnv *env, jobject instance, jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    if (!videoChannel || !readyPushing) {
        return;
    }
    videoChannel->encodeData(data);

    env->ReleaseByteArrayElements(data_, data, 0);
}