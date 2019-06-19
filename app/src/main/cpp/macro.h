

#ifndef DNPLAYER_MACRO_H
#define DNPLAYER_MACRO_H
#include <android/log.h>
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"MyPush",__VA_ARGS__)
//宏函数
#define DELETE(obj) if(obj){ delete obj; obj = 0; }

#endif //DNPLAYER_MACRO_H
