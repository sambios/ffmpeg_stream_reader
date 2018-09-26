#include "netstream_reader.h"
#include "opencv2/opencv.hpp"
#include "face_info.h"
#include "getopt.h"
#include "h264sei.h"

class TestVideoObserver :public NetStreamReaderObserver 
{
    cv::Mat m_image;
public: 
    TestVideoObserver(){}
	~TestVideoObserver(){}

	void OnDecodedFrame(const AVFrame *pFrame) override {
        m_image = avframe_to_cvmat(pFrame);

	}

    void OnSeiReceived(uint8_t *buff, uint32_t size) {
        int face_num = buff[0];
        cv::Mat img = m_image.t();
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

            }
            else {
                cv::rectangle(img, cvRect, cv::Scalar(255, 0, 255), 2, 1, 0);
            }
        }


        cv::imshow("test", img.t());
        cv::waitKey(20);
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

	NetStreamReader reader;
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
   
	int ret = reader.OpenStream(input_url, &observer);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		std::this_thread::sleep_for(10 * std::chrono::seconds());
	}
       
        ret = reader.CloseStream();
        return 0;
}




