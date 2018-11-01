#include "netstream_reader.h"
#include "opencv2/opencv.hpp"
#include "face_info.h"
#include "getopt.h"
#include "h264sei.h"


struct TestFaceInfo {
    int64_t pts;
    int num;
    cv::Rect rects[16];
};

struct TestFrame {
    cv::Mat img;
    int64_t pts;
};


void pase_face_info(uint8_t *buff, int size, TestFaceInfo &faceinfo)
{
    //TestFaceInfo faceinfo;
    if (size == 0) {
        faceinfo.num = 0;
        return;
    }
    int face_num = buff[0];
    faceinfo.pts = 0;
    faceinfo.num = face_num;

    int offset = 1;
    for (int i = 0; i < face_num; ++i) {
        FaceRect rc;
        int len = rc.UnPack(&buff[offset], size);
        offset += len;
        cv::Rect cvRect;
        cvRect.x = rc.x;
        cvRect.y = rc.y;
        cvRect.width = rc.width;
        cvRect.height = rc.height;
        if (rc.x < 0 || rc.y < 0 || rc.width > 480 || rc.height > 640) {
            printf("zuobiao invalid.\n");
        }
        else {
            //cv::rectangle(img, cvRect, cv::Scalar(255, 0, 255), 2, 1, 0);
            faceinfo.rects[i] = cvRect;
        }
    }
}

cv::Mat to_cvmat(uint8_t *src_data[4], int src_linesize[4], int src_w, int src_h)
{
    uint8_t /**src_data[4],*/ *dst_data[4];
    int /*src_linesize[4],*/ dst_linesize[4];
    int dst_w, dst_h;

    struct SwsContext *convert_ctx = NULL;
    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;


    dst_w = src_w;
    dst_h = src_h;

    cv::Mat m;
    m = cv::Mat(dst_h, dst_w, CV_8UC3);
    av_image_fill_arrays(dst_data, dst_linesize, m.data, dst_pix_fmt, dst_w, dst_h, 16);

    convert_ctx = sws_getContext(src_w, src_h, src_pix_fmt, dst_w, dst_h, dst_pix_fmt,
                                 SWS_FAST_BILINEAR, NULL, NULL, NULL);

    sws_scale(convert_ctx, src_data, src_linesize, 0, dst_h,
              dst_data, dst_linesize);

    sws_freeContext(convert_ctx);

    return m;
}


class TestVideoObserver :public NetStreamReaderObserver 
{
    std::list<TestFaceInfo> m_listFaceInfos;
    std::list<TestFrame> m_listImages;

public: 
    TestVideoObserver(){}
	~TestVideoObserver(){}

        void OnDecodedFrame(uint8_t *planes[3], int linesize[3], int w, int h, int64_t pts) override
        {
            //static int cacheCount = 7;

            TestFrame frame;
            cv::Mat img = to_cvmat(planes, linesize, w, h);

            frame.img = img;
            frame.pts = pts;
            m_listImages.push_back(frame);

            if (m_listImages.size() < 1) {
                //cacheCount++;
                return;
            }

            //TestFrame frame;
            //cv::Mat img = to_cvmat(planes, linesize, w, h);

            //printf("faceinfo num=%d\n", m_listFaceInfos.size());
            if (m_listImages.size() > 0) {
                auto frame = m_listImages.front();
                m_listImages.pop_front();
                TestFaceInfo faceInfo;
                while (m_listFaceInfos.size() > 0) {
                    faceInfo = m_listFaceInfos.front();
                    if (1) {
                        printf("facePTS - framePTS=%d\n", faceInfo.pts - frame.pts);
                        for (int i = 0; i < faceInfo.num; ++i) {
                            cv::rectangle(frame.img, faceInfo.rects[i], cv::Scalar(255, 0, 255), 2, 1, 0);
                        }
                        m_listFaceInfos.pop_front();
                    }
                    else {
                        break;
                    }
                }


                cv::imshow("test", frame.img);
                cv::waitKey(1);
            }
            else {
                printf("no sei\n");
            }


        }

    void OnSeiReceived(uint8_t *buff, uint32_t size, int64_t pts) {
        
        int face_num = buff[0];
        m_listFaceInfos.clear();
      
        TestFaceInfo faceinfo;
        faceinfo.pts = pts;
        faceinfo.num = face_num;

        int offset = 1;
        for (int i = 0; i < face_num; ++i) {
            FaceRect rc;
            int len = rc.UnPack(&buff[offset], size);
            offset += len;
            cv::Rect cvRect;
            cvRect.x = rc.x;
            cvRect.y = rc.y;
            cvRect.width = rc.width;
            cvRect.height = rc.height;
            if (rc.x < 0 || rc.y < 0 || rc.width > 480 || rc.height > 640) {
                printf("zuobiao invalid.\n");
            }
            else {
                //cv::rectangle(img, cvRect, cv::Scalar(255, 0, 255), 2, 1, 0);
                faceinfo.rects[i] = cvRect;
            }
        }

        if (face_num > 0)
        m_listFaceInfos.push_back(faceinfo);

        
    }
};

void dump_array(uint8_t *buff, int len)
{
    for (int i = 0; i < len; ++i)
    {
        printf("%x ", buff[i]);
    }

    printf("\n");

}

void test_pack()
{
    uint8_t buff[100];

    uint32_t a = 0x11223344;

    uint32_t b = htobe32(a);

    printf("HOST:0x%x\n", a);
    printf("BE:0x%x\n", b);
    
    memcpy(buff, &a, sizeof(a));
    dump_array(buff, 4);

    memcpy(buff, &b, sizeof(b));
    dump_array(buff, 4);

    uint32_t c;
    memcpy(&c, buff, 4);
    uint32_t d = be32toh(c);
    printf("d=0x%x\n", d);

    FaceRect rc;
    rc.x = 1;
    rc.y = 1;
    rc.width = 100;
    rc.height = 100;

    int len = rc.Pack(buff, 100);

    uint8_t pkt_buf[200], sei_msg[200];
    int sei_size = h264sei_packet_write(pkt_buf, true, buff, len);
    h264sei_packet_read(pkt_buf, sei_size, sei_msg, 200);
   
    FaceRect rc2;
    rc2.UnPack(sei_msg, len);

    assert(rc.x == rc2.x);
    assert(rc.y == rc2.y);
    assert(rc.width == rc2.width);
    assert(rc.height == rc2.height);

}



int main(int argc, char *argv[])
{
    test_pack();

    std::shared_ptr<NetStreamReader> reader = NetStreamReader::create(1);

    TestVideoObserver observer;
    std::string bmctx_path, input_url, output_url;
    int ch = 0;
    while ((ch = getopt(argc, argv, "c:i:o:")) != -1)
    {
        switch (ch)
        {
        case 'c':
            printf("HAVE option: -b\n");
            printf("The argument of -b is %s\n\n", optarg);
            bmctx_path = optarg;
            break;
        case 'i':
            printf("HAVE option: -b\n");
            printf("The argument of -b is %s\n\n", optarg);
            input_url = optarg;
            break;
        case 'o':
            printf("HAVE option: -c\n");
            printf("The argument of -c is %s\n\n", optarg);
            output_url = optarg;
            break;
        }
    }
   
	int ret = reader->OpenStream(input_url, &observer);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		std::this_thread::sleep_for(10 * std::chrono::seconds());
	}
       
        ret = reader->CloseStream();
        return 0;
}




