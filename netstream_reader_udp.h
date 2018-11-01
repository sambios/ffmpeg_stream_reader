#pragma once

#include "apl.h"
#include "e3h264rtp.h"

class NetStreamReaderUdp : public NetStreamReader, public e3lib::h264_rtp_receiver_observer {
   
    int m_state;
    NetStreamReaderObserver *m_observer;
    bool m_bRunning{ false };

    e3lib::h264_rtp_receiver *m_receiver;
    handle_t m_h264dec;
public:
    NetStreamReaderUdp();
    virtual ~NetStreamReaderUdp();

    int OpenStream(std::string strUrl, NetStreamReaderObserver *observer) override;
    int CloseStream() override;

    // implements
    virtual void on_receive_frame(uint32_t ssrc, uint32_t ts, uint8_t *frame_buf, int len) override;

private:
   
    std::thread *m_decodeThread;

};