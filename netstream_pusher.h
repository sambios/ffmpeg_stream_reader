#pragma once

#include "opencv2/opencv.hpp"

#ifdef __cplusplus
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
}
#endif 

#include <thread>

class NetStreamPusher
{
	AVFormatContext *m_ofmtCtx;
	AVCodec *m_h264Codec;
	AVCodecContext *m_h264Enc;
	bool m_bOutputConnected = false;
	std::string m_url;
	AVStream *out_stream;
        uint8_t *m_encoded_buff = nullptr;
	int PushPacket(AVPacket *pkt);
	int CreateEncoder(int width, int height);
	std::list<AVPacket*> m_packet_list;
	std::thread *m_pushThread = nullptr;

public:
	NetStreamPusher(const std::string &url);
	~NetStreamPusher();

	virtual int PushData(const cv::Mat &image);
	int WritePacket(AVPacket *pkt);
	int PushOnePacket(AVPacket *pkt);
};

