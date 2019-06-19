package com.yu.mypush;

import android.app.Activity;
import android.view.SurfaceHolder;

import com.yu.mypush.media.AudioChannel;
import com.yu.mypush.media.VideoChannel;

public class LivePusher {
    private AudioChannel audioChannel;
    private VideoChannel videoChannel;
    static {
        System.loadLibrary("native-lib");
    }

    public LivePusher(Activity activity, int width, int height, int bitrate,
                      int fps, int cameraId) {
        native_init();
        videoChannel = new VideoChannel(this, activity, width, height, bitrate, fps, cameraId);
        audioChannel = new AudioChannel(this);
    }

    /**
     * 设置摄像头预览
     * @param surfaceHolder
     */
    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        videoChannel.setPreviewDisplay(surfaceHolder);
    }

    /**
     * 切换摄像头
     */
    public void switchCamera() {
        videoChannel.switchCamera();
    }

    /**
     * 开始直播
     * @param path
     */
    public void startLive(String path) {
        native_start(path);
        videoChannel.startLive();
    }

    /**
     * 停止直播
     */
    public void stopLive() {
        videoChannel.stopLive();
    }

    public native void native_init();

    public native void native_setVideoEncInfo(int w, int h, int mFps, int mBitrate);

    public native void native_start(String path);

    public native void native_pushVideo(byte[] data);

}
