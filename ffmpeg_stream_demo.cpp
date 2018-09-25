#include "netstream_reader.h"
#include "opencv2/opencv.hpp"
#include "face_info.h"

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
        cv::waitKey(10);
    }
};



int main()
{
	NetStreamReader reader;
    TestVideoObserver observer;
	int ret = reader.OpenStream("rtsp://10.30.70.21:8554/test_sei", &observer);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		std::this_thread::sleep_for(10 * std::chrono::seconds());
	}
       
        ret = reader.CloseStream();
        return 0;
}




