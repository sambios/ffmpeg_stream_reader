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
	virtual void OnDecodedFrame(uint8_t *planes[3], int linesize[3], int w, int h, int64_t pts) = 0;
        virtual void OnSeiReceived(uint8_t *buff, uint32_t size, int64_t pts) = 0;
};

class NetStreamReader {
public:
    static std::shared_ptr<NetStreamReader> create(int type);

    virtual ~NetStreamReader() {};
    virtual int OpenStream(std::string strUrl, NetStreamReaderObserver *observer) = 0;
    virtual int CloseStream() = 0;
};


