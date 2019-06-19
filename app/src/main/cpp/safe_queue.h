//
// Created by liuxiang on 2017/10/15.
//

#ifndef DNRECORDER_SAFE_QUEUE_H
#define DNRECORDER_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>
using namespace std;

template<typename T>
class SafeQueue {
public:
    SafeQueue() {
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
    }

    ~SafeQueue() {
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
    }
    void put( T new_value) {
        //锁 和智能指针原理类似，自动释放
        pthread_mutex_lock(&mutex);
        if (work) {
            q.push(new_value);
            pthread_cond_signal(&cond);
        }else{
        }
        pthread_mutex_unlock(&mutex);
    }


    int get(T& value) {
        int ret = 0;
        pthread_mutex_lock(&mutex);
        //在多核处理器下 由于竞争可能虚假唤醒 包括jdk也说明了
        while (work && q.empty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        if (!q.empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
        return ret;
    }

    void setWork(int work) {
        pthread_mutex_lock(&mutex);
        this->work = work;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    int empty() {
        return q.empty();
    }

    int size() {
        return q.size();
    }

    void clear() {
        pthread_mutex_lock(&mutex);
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            q.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    void sync() {
        pthread_mutex_lock(&mutex);
        //同步代码块 当我们调用sync方法的时候，能够保证是在同步块中操作queue 队列
        pthread_mutex_unlock(&mutex);
    }
private:
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    queue<T> q;
    //是否工作的标记 1 ：工作 0：不接受数据 不工作
    int work;
};


#endif //DNRECORDER_SAFE_QUEUE_H
