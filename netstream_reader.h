#pragma once

#include <iostream>
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include "libavutil/time.h"
}

#include "singleton_template.h"

enum StreamState {
	StreamStateStopped = 0,
	StreamStateStarted,
};

class FfmpegGlobal {
public:
	FfmpegGlobal() {
		avformat_network_init();
	}

	~FfmpegGlobal() {
		avformat_network_deinit();
	}
};


class NetStreamReaderObserver {
public:
	virtual ~NetStreamReaderObserver() {}
	virtual void OnDecodedFrame(const AVFrame* frame) = 0;
};

class NetStreamReader {
public:
    static std::shared_ptr<NetStreamReader> create(int type);

    virtual ~NetStreamReader() {};
    virtual int OpenStream(std::string strUrl, NetStreamReaderObserver *observer) = 0;
    virtual int CloseStream() = 0;
};


