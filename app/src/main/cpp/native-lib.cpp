#include <jni.h>
#include <string>
#include "x264.h"
#include "librtmp/rtmp.h"
#include "VideoChannel.h"
#include "pthread.h"
#include "macro.h"
#include "safe_queue.h"

VideoChannel *videoChannel;//编码专用，会回调编码之后的RTMPPacket
int isStart = 0;//为了防止用户重复点击开始直播，导致重新初始化
pthread_t pid; //连接服务器的线程
uint32_t start_time;//开始推流时间戳
int readyPushing = 0;// 是否已连接服务器，准备就绪
SafeQueue<RTMPPacket *> packets;//队列，用于存储VideoChannel中组装好准备传输的RTMPPacket


/**
 * VideoChannel的回调方法，会收到VideoChannel中编码之后的每个RTMPPacket，计入队列等待推流（上传服务器）
 * @param packet
 */
void callback(RTMPPacket *packet) {

    if (packet) {

        //设置时间戳
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        //加入队列
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
 * 该方法在开始直播的方法(Java_com_yu_mypush_LivePusher_native_1start)中调用，可以理解为Java里new Thread中的run()方法
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
/**
 * 初始化
 * @param env
 * @param instance
 */
Java_com_yu_mypush_LivePusher_native_1init(JNIEnv *env, jobject instance) {

    videoChannel = new VideoChannel;
    //设置回调，因为VideoChannel只负责编码，这里拿到编码之后的数据进行传输
    videoChannel->setVideoCallback(callback);
}


extern "C"
JNIEXPORT void JNICALL
/**
 * 初始化视频数据，在摄像头数据尺寸变化时Java层调用的（第一次打开摄像头、摄像头切换、横竖屏切换都会引起摄像头采集的尺寸发送变化，会走这个方法）
 * @param env
 * @param instance
 * @param width
 * @param height
 * @param fps
 * @param bitrate
 */
Java_com_yu_mypush_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject instance, jint width, jint height,
                                                      jint fps, jint bitrate) {

    if (!videoChannel) {
        return;
    }
    videoChannel->setVideoEncInfo(width, height, fps, bitrate);

}


extern "C"
JNIEXPORT void JNICALL
/**
 * 开始直播（连接服务器，从队列中取出数据并推向服务器）
 * 该方法会在点击开始直播之后调用
 * @param env
 * @param instance
 * @param path_
 */
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
/**
 * 开始数据编码（开始后会收到每个编码后的packet回调并加入到队列中）
 * 该方法会在开始直播并且收到摄像头返回数据之后调用
 * @param env
 * @param instance
 * @param data_
 */
Java_com_yu_mypush_LivePusher_native_1pushVideo(JNIEnv *env, jobject instance, jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    if (!videoChannel || !readyPushing) {
        return;
    }
    videoChannel->encodeData(data);

    env->ReleaseByteArrayElements(data_, data, 0);
}