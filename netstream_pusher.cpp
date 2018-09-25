
//

#include <iostream>
#include "netstream_pusher.h"
#ifdef __cplusplus
extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/time.h>
}
#endif //!__cplusplus

AVRational instream_time_base = { 1, 1000 };
AVRational outstream_time_base = { 1, 90000 };
int frame_index = 0;

static cv::Mat avframe_to_cvmat(AVFrame *src)
{
    uint8_t /**src_data[4],*/ *dst_data[4];
    int /*src_linesize[4],*/ dst_linesize[4];
    int src_w = 320, src_h = 240, dst_w, dst_h;

    struct SwsContext *convert_ctx = NULL;
    enum AVPixelFormat src_pix_fmt = (enum AVPixelFormat)src->format;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;


    src_w = dst_w = src->width;
    src_h = dst_h = src->height;

    cv::Mat m;
    m = cv::Mat(dst_h, dst_w, CV_8UC3);
    av_image_fill_arrays(dst_data, dst_linesize, m.data, dst_pix_fmt, dst_w, dst_h, 16);

    convert_ctx = sws_getContext(src_w, src_h, src_pix_fmt, dst_w, dst_h, dst_pix_fmt,
        SWS_FAST_BILINEAR, NULL, NULL, NULL);

    sws_scale(convert_ctx, src->data, src->linesize, 0, dst_h,
        dst_data, dst_linesize);

    sws_freeContext(convert_ctx);

    return m;
}

static AVFrame* cvmat_to_avframe(const cv::Mat* src, int to_pix_fmt)
{
    uint8_t *src_data[4]/*, *dst_data[4]*/;
    int src_linesize[4]/*, dst_linesize[4]*/;
    int src_w = 320, src_h = 240, dst_w, dst_h;

    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_BGR24;
    enum AVPixelFormat dst_pix_fmt = (enum AVPixelFormat)to_pix_fmt;
    struct SwsContext *convert_ctx = NULL;

    src_w = dst_w = src->cols;
    src_h = dst_h = src->rows;

    convert_ctx = sws_getContext(src_w, src_h, src_pix_fmt, dst_w, dst_h, (enum AVPixelFormat)dst_pix_fmt,
        SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (nullptr == convert_ctx) {
        printf("sws_getContext failed!");
        return nullptr;
    }


    av_image_fill_arrays(src_data, src_linesize, src->data, src_pix_fmt, src_w, src_h, 16);

    AVFrame *dst = av_frame_alloc();
    av_image_alloc(dst->data, dst->linesize, dst_w, dst_h, (enum AVPixelFormat) dst_pix_fmt, 16);
   
    dst->format = dst_pix_fmt;
    dst->width = dst_w;
    dst->height = dst_h;
    //
    int ret = sws_scale(convert_ctx, src_data, src_linesize, 0, dst_h,
        dst->data, dst->linesize);
    if (ret < 0) {
        printf("sws_scale err\n");
    }

    sws_freeContext(convert_ctx);

    return dst;
}


NetStreamPusher::NetStreamPusher(const std::string& url)
{
    
    m_url = url;
    m_h264Enc = nullptr;
    out_stream = nullptr;

    avformat_network_init();
    m_encoded_buff = (uint8_t *)av_malloc(512<<10);
}


NetStreamPusher::~NetStreamPusher()
{
    av_free(m_encoded_buff);
    avformat_network_deinit();
}

int NetStreamPusher::CreateEncoder(int width, int height)
{
    int ret = 0;
    m_h264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    assert(m_h264Codec != nullptr);

    m_h264Enc = avcodec_alloc_context3(m_h264Codec);
    assert(m_h264Enc != nullptr);

    /* put sample parameters */
    m_h264Enc->bit_rate = 512 << 10;
    /* resolution must be a multiple of two */
    m_h264Enc->width = width;
    m_h264Enc->height = height;

    /* frames per second */
    m_h264Enc->time_base.den = 25;
    m_h264Enc->time_base.num = 1;
    m_h264Enc->framerate.num = 25;
    m_h264Enc->framerate.den = 1;

    m_h264Enc->gop_size = 25; /* emit one intra frame every ten frames */
    m_h264Enc->max_b_frames = 0;
    m_h264Enc->pix_fmt = (enum AVPixelFormat) AV_PIX_FMT_YUV420P;
    m_h264Enc->qmax = 2;
    m_h264Enc->qmin = 32;
    m_h264Enc->delay = 0;

    AVDictionary *options = NULL;
    av_dict_set(&options, "preset", "medium", 0);
    av_dict_set(&options, "tune", "zerolatency", 0);
    av_dict_set(&options, "profile", "baseline", 0);


    ret = avcodec_open2(m_h264Enc, m_h264Codec, NULL);
    assert(ret == 0);

    return 0;
}


int NetStreamPusher::PushData(const cv::Mat& mat)
{
    int ret = 0;
    
    cv::Mat mat2;
    AVFrame *frame = cvmat_to_avframe(&mat, AV_PIX_FMT_YUV420P);
    frame->pts = av_gettime_relative()/1000;
    if (!m_h264Enc) {
        ret = CreateEncoder(frame->width, frame->height);
        if (ret < 0) {
            printf("Create encoder failed!\n");
            return -1;
        }
    }

    //mat2 = avframe_to_cvmat(frame);
    //cv::imshow("test", mat2);

    
    int got_packet = 0;
    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);
    
    //pkt->data = m_encoded_buff;
    //pkt->size = (512<<10);
    
    if (m_h264Enc != nullptr) {

        // for ffmpeg 3.4.1
        /*
        ret = avcodec_send_frame(m_h264Enc, frame);
        if (ret < 0) {
            printf("avcodec_send_frame err=%d\n", ret);
            return ret;
        }

        ret = avcodec_receive_packet(m_h264Enc, pkt);
        if (ret < 0) {
            //printf("avcodec_receive packet err=%d\n", ret);
            return ret;
        }
        */

        ret = avcodec_encode_video2(m_h264Enc, &pkt, frame, &got_packet);
        if (ret < 0) {
            printf("avcodec_encode_video2 err\n");
        }
        if (got_packet) {

		//printf("[netstream] pushpacket!\n");
                PushPacket(&pkt);
        }
    }

    
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    av_packet_unref(&pkt);

    return 0;
}

int NetStreamPusher::PushPacket(AVPacket *inpkt)
{
    AVPacket *pkt = av_packet_clone(inpkt);
    pkt->stream_index = 0;
    m_packet_list.push_back(pkt);

    if (m_pushThread == nullptr) {
        m_pushThread = new std::thread([&] {
            while (1) {

                if (m_packet_list.size() > 0) {
		    //printf("[netstream]write packet!\n");
                    AVPacket *p = m_packet_list.front();
                    WritePacket(p);
                    av_packet_unref(p);
                    av_packet_free(&p);
                    m_packet_list.pop_front();
                }else{
		   av_usleep(10000);
		}
            }
            //std::this_thread::sleep_for(10 * std::chrono::milliseconds());
        });
    }

    return 0;
}

int NetStreamPusher::WritePacket(AVPacket *pkt)
{
    int ret = 0;
    if (!m_bOutputConnected) {
        avformat_alloc_output_context2(&m_ofmtCtx, nullptr, "rtsp", m_url.c_str());
        if (!m_ofmtCtx) {
            printf("av_alloc_output_context2 failed\n");
            ret = AVERROR_UNKNOWN;
            return -1;
        }

        auto ofmt = m_ofmtCtx->oformat;

        for (int i = 0; i < 1; i++) {
            out_stream = avformat_new_stream(m_ofmtCtx, m_h264Codec);
            if (!out_stream) {
                printf("can't create stream\n");
                ret = AVERROR_UNKNOWN;
                return -1;
            }

            //Copy the settings of AVCodecContext
            ret = avcodec_parameters_from_context(out_stream->codecpar, m_h264Enc);
            //ret = avcodec_copy_context(out_stream->codec, m_h264Enc);
            if (ret < 0) {
                printf("Failed to copy context from input to output stream codec context\n");
                return -1;
            }
            out_stream->codecpar->codec_tag = 0;
            if (m_ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codecpar->codec_tag |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        //检查参数设置有无问题
        av_dump_format(m_ofmtCtx, 0, m_url.c_str(), 1);

        //打开输出地址（推流地址）
        if (!(ofmt->flags & AVFMT_NOFILE)) {
            ret = avio_open(&m_ofmtCtx->pb, m_url.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                printf("无法打开地址 '%s'", m_url.c_str());
                return -1;
            }
        }

        //写视频文件头
	printf("[netstream] write header\n");
        ret = avformat_write_header(m_ofmtCtx, NULL);
        if (ret < 0) {
            printf("在 URL 所在的文件写视频头出错\n");
            return -1;
        }

        m_bOutputConnected = true;
    }

    static int64_t start_time = 0;
    if (start_time == 0) {
        start_time = av_gettime();
    }

    //printf("[netstream]1111111111111111\n");
    if (pkt->stream_index > 0) return -1;

    //Important:Delay
    if (pkt->stream_index == 0) {
        AVRational time_base = { 1, 1000};
        AVRational time_base_q = { 1, AV_TIME_BASE };
        int64_t pts_time = av_rescale_q(pkt->pts, time_base, time_base_q);
        int64_t now_time = av_gettime() - start_time;
        //if (pts_time > now_time)
        //    av_usleep(pts_time - now_time);

    }

    //printf("[netstream]22222222222222\n");
    /* copy packet */
    //Convert PTS/DTS
    pkt->pts = av_rescale_q_rnd(pkt->pts, instream_time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt->dts = av_rescale_q_rnd(pkt->dts, instream_time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt->duration = av_rescale_q(pkt->duration, instream_time_base, out_stream->time_base);
    pkt->pos = -1;

    //Print to Screen
    if (pkt->stream_index == 0) {
        printf("Send %8d video frames to output URL\n", frame_index);
        frame_index++;
    }

    printf("pkt.pts = %lld, pkt.dts=%lld, pkt.duration=%lld\n", pkt->pts, pkt->dts, pkt->duration);
    ret = av_interleaved_write_frame(m_ofmtCtx, pkt);

    if (ret < 0) {
        printf("Error muxing packet\n");
    }

    return 0;

}
