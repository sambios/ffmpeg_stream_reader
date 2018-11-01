#include "netstream_reader.h"
#include "netstream_reader_udp.h"
#include <iostream>


NetStreamReaderUdp::NetStreamReaderUdp():m_receiver(nullptr),
m_observer(nullptr)
{
    m_h264dec = ffmpeg_dec_create(AV_CODEC_ID_H264, false);
}

NetStreamReaderUdp::~NetStreamReaderUdp()
{

}

int NetStreamReaderUdp::OpenStream(std::string strUrl, NetStreamReaderObserver *observer)
{
    size_t pos = strUrl.find("rtp://", 0);
    if (std::string::npos == pos) {
        printf("can't find 'rtp://' string!\n");
        return -1;
    }

    pos = strUrl.rfind(':');
    if (std::string::npos == pos) {
        printf("can't find port!\n");
        return -1;
    }

    std::string port_str = strUrl.substr(pos+1);
    uint16_t port = atoi(port_str.c_str());
    if (0 == port) {
        return -2;
    }

    if (m_receiver == nullptr) {
        m_receiver = e3lib::h264_rtp_receiver::create();
        m_receiver->set_observer(this);
    }

    m_receiver->init(port);
    m_receiver->start_receive();
    m_observer = observer;

    return 0;
}

int NetStreamReaderUdp::CloseStream()
{
    if (m_receiver) {
        m_receiver->stop_receive();
    }
    return 0;
}

void NetStreamReaderUdp::on_receive_frame(uint32_t ssrc, uint32_t ts, uint8_t *frame_buf, int len)
{
    std::cout << "recv frame_len=" << len << std::endl;

    if (0 == frame_buf[0] && 0 == frame_buf[1] && 0 == frame_buf[2] && 1 == frame_buf[3]) {
        if ((frame_buf[4] & 0x1f) == 6) {
            u8 sei_buff[512] = { 0 };

            int sei_len = apl_h264_sei_read(frame_buf, len, sei_buff, 512);

            if (m_observer) {
                m_observer->OnSeiReceived(sei_buff, sei_len, ts);
            }

        }
    }

    ffmpeg_dec_frame_t frameParam;
    memset(&frameParam, 0, sizeof(frameParam));
    frameParam.pu8Bits = frame_buf;
    frameParam.s32BitsLen = len;
   
    ffmpeg_dec_decode(m_h264dec, &frameParam);

    if (m_observer && frameParam.got_picture) {

        m_observer->OnDecodedFrame(frameParam.pu8YUVSlice, frameParam.linesize, frameParam.s32Width,
                                   frameParam.s32Height, ts);
    }
}