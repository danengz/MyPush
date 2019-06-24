package com.yu.mypush.media;


import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import com.yu.mypush.LivePusher;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AudioChannel {
    private LivePusher mLivePusher;
    private AudioRecord audioRecord;
    private int inputSamples;
    private int channels = 2;//双声道
    int channelConfig;
    int minBufferSize;
    private ExecutorService executor;
    private boolean isLiving;
    public AudioChannel(LivePusher livePusher) {
        executor = Executors.newSingleThreadExecutor();
        mLivePusher = livePusher;
        if (channels == 2) {
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }
        mLivePusher.native_setAudioEncInfo(44100, channels);

        minBufferSize = AudioRecord.getMinBufferSize(44100,
                channelConfig, AudioFormat.ENCODING_PCM_16BIT);

        //faac返回的当前输入采样率对应的采样个数,因为是用的16位，所以*2是byte长度
        inputSamples = mLivePusher.getInputSamples() * 2;

        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, 44100, channelConfig,
                AudioFormat.ENCODING_PCM_16BIT, Math.min(minBufferSize, inputSamples));
    }

    public void startLive() {
        isLiving = true;
        executor.submit(new AudioTeask());
    }



    public void setChannels(int channels) {
        this.channels = channels;
    }

    class AudioTeask implements Runnable {


        @Override
        public void run() {
            audioRecord.startRecording();

            byte[] bytes = new byte[inputSamples];
            while (isLiving) {
                int len = audioRecord.read(bytes, 0, bytes.length);
                mLivePusher.native_pushAudio(bytes);
            }
        }
    }

    public void stopLive() {
        isLiving = false;
    }
}
