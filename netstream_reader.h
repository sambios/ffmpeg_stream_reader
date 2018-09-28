#pragma once

#include <iostream>
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
}

#include "singleton_template.h"

enum StreamState {
	StreamStateStopped = 0,
	StreamStateStarted,
};

class FfmpegGlobal {
public:
	FfmpegGlobal() {
		av_register_all();
		avformat_network_init();
	}

	~FfmpegGlobal() {
		avformat_network_deinit();
	}
};


class NetStreamReaderObserver {
public:
	virtual ~NetStreamReaderObserver() {}
	virtual void OnDecodedFrame(const AVFrame *pFrame) = 0;
    virtual void OnSeiReceived(uint8_t *buff, uint32_t size, int64_t pts) = 0;
};

class NetStreamReader {
	FfmpegGlobal *m_ffmpegGlobal;
	int m_state;
	NetStreamReaderObserver *m_observer;
public:
	NetStreamReader();
	virtual ~NetStreamReader();

	int OpenStream(std::string strUrl, NetStreamReaderObserver *observer);
	int CloseStream();
private:
	AVFormatContext *m_pFormatCtx;
	AVCodecContext *m_pCodecCtx;
	AVFrame *m_pFrame;
	AVPacket m_packet;
	AVPicture  m_pAVPicture;
	SwsContext * m_pSwsCtx;
	// Video
	int m_videoStreamIndex;
	int m_width;
	int m_height;

	std::thread *m_decodeThread;
    
};

