#pragma once

#include "netstream_reader.h"

class NetStreamReaderFfmpeg : public NetStreamReader {
    FfmpegGlobal *m_ffmpegGlobal;
    int m_state;
    NetStreamReaderObserver *m_observer;
    bool m_bRunning{ false };
public:
    NetStreamReaderFfmpeg();
    virtual ~NetStreamReaderFfmpeg();

    int OpenStream(std::string strUrl, NetStreamReaderObserver *observer) override;
    int CloseStream() override;

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
