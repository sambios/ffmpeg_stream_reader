
#include <cstdlib>
#include <thread>
#include <iostream>

#include "netstream_reader.h"



class VideoObserver :public NetStreamReaderObserver
{
public: 
	VideoObserver(){}
	~VideoObserver(){}

	void OnDecodedFrame(const AVFrame *pFrame) override {
		std::cout << "w=" << pFrame->width
			<< ",h=" << pFrame->height
			<< std::endl;
	}
};



int main(int argc, char *argv[])
{
	if (argc <= 1) {
		std::cout << "missing parameter: rtsp URL" << std::endl;
		return 0;
	}

	std::shared_ptr<NetStreamReader> reader(NetStreamReader::create(0));
	VideoObserver observer;
	int ret = reader->OpenStream(argv[1], &observer);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}


}