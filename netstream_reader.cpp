#include "pch.h"
#include "netstream_reader.h"
#include "h264sei.h"


static int ffmpeg_interrupt_cb(void* p)
{
	//printf("#### ERROR ####\n");
	return 0;
}


NetStreamReader::NetStreamReader() {
	m_ffmpegGlobal = Singleton<FfmpegGlobal>::instance();
	m_pCodecCtx = NULL;
	m_videoStreamIndex = -1;
	m_pFormatCtx = avformat_alloc_context();
    m_pSwsCtx = nullptr;
	
	AVIOInterruptCB cb;
	cb.callback = ffmpeg_interrupt_cb;
	cb.opaque = this;
	m_pFormatCtx->interrupt_callback = cb;
	
	m_pFrame = av_frame_alloc();
}

NetStreamReader::~NetStreamReader() {
	avformat_free_context(m_pFormatCtx);
	av_frame_free(&m_pFrame);
    if (m_pSwsCtx) sws_freeContext(m_pSwsCtx);
}

int NetStreamReader::OpenStream(std::string strUrl, NetStreamReaderObserver *observer)
{
	m_observer = observer;
	AVDictionary* options = NULL;
	av_dict_set(&options, "rtsp_transport", "tcp", 0);
	int err = avformat_open_input(&m_pFormatCtx, strUrl.c_str(), NULL, &options);
	if (err < 0)
	{
		std::cout << "Can not open this file" << std::endl;
		return -1;
	}

	av_dict_free(&options);

	err = avformat_find_stream_info(m_pFormatCtx, NULL);
	if (err < 0)
	{
		std::cout << "Unable to get stream info";
		return -1;
	}

	m_videoStreamIndex = -1;
	for (unsigned int i = 0; i < m_pFormatCtx->nb_streams; i++)
	{
		if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videoStreamIndex = i;
			break;
		}
	}

	if (m_videoStreamIndex == -1)
	{
		std::cout << "Unable to find video stream";
		return -1;
	}

	m_pCodecCtx = m_pFormatCtx->streams[m_videoStreamIndex]->codec;
	m_width = m_pCodecCtx->width;
	m_height = m_pCodecCtx->height;

	avpicture_alloc(&m_pAVPicture, AV_PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height);

	AVCodec *pCodec;
	pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	m_pSwsCtx = sws_getContext(m_width, m_height, AV_PIX_FMT_YUV420P, m_width, m_height, AV_PIX_FMT_RGB24, SWS_BICUBIC, 0, 0, 0);
	if (pCodec == NULL)
	{
		std::cout << "Unsupported codec";
		return -1;
	}

	//std::cout << QString("video size : width=%d height=%d \n").arg(pCodecCtx->width).arg(pCodecCtx->height);

	if (avcodec_open2(m_pCodecCtx, pCodec, NULL) < 0)
	{
		std::cout << "Unable to open codec";
		return -1;
	}

	std::cout << "initial successfully";
	m_state = StreamStateStarted;
	// Decoder thread
	m_decodeThread = new std::thread([=] {
		int ret = 0;
		int got_pictrue = 0;
		while (true) {
			if (m_state == StreamStateStopped) {
				break;
			}

			ret = av_read_frame(m_pFormatCtx, &m_packet);
			if (ret < 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds() * 10);
				continue;
			}

			// handle video stream
			if (m_packet.stream_index == m_videoStreamIndex)
			{
                if ((m_packet.data[4] & 0x1f) == 6) {
                    if (m_observer != nullptr) {
                        uint8_t sei_buf[256];
                        int len = h264sei_packet_read(m_packet.data, m_packet.size, sei_buf, sizeof(sei_buf));
                        if (len > 0) {
                            m_observer->OnSeiReceived(sei_buf, len, m_packet.pts);
                        }
                    }
                }

				ret = avcodec_send_packet(m_pCodecCtx, &m_packet);
				if (ret < 0) {
					std::cout << "avcodec_send_packet error=" << ret << std::endl;
				}

				ret = avcodec_receive_frame(m_pCodecCtx, m_pFrame);
				if (ret < 0) {
					std::cout << "avcodec_send_packet error=" << ret << std::endl;
				}
				else {
					if (m_observer != nullptr) {
						m_observer->OnDecodedFrame(m_pFrame);
					}
				}


				//avcodec_decode_video2(m_pCodecCtx, m_pFrame, &got_pictrue, &m_packet);
				/*if (got_pictrue)
				{
					printf("***************ffmpeg decodec*******************\n");
					AVStream *stream = m_pFormatCtx->streams[m_packet.stream_index];
					
					std::cout << "w=" << stream->codecpar->width
						<< ",h=" << stream->codecpar->height
						<< std::endl;
				}*/
			}
			av_free_packet(&m_packet);
		}
	});


	return 0;

}


int NetStreamReader::CloseStream()
{
	m_state = StreamStateStopped;
	m_decodeThread->join();
	delete m_decodeThread;

	return 0;
}
