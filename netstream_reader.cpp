#include "pch.h"
#include "netstream_reader.h"
#include "h264sei.h"
#include "netstream_reader_udp.h"
#include "netstream_reader_ffmpeg.h"

std::shared_ptr<NetStreamReader> NetStreamReader::create(int type)
{
    std::shared_ptr<NetStreamReader> ptr;
    switch (type) {
    case 0:
        ptr.reset(new NetStreamReaderFfmpeg());
        break;
    case 1:
        ptr.reset(new NetStreamReaderUdp());
        break;
    }

    return ptr;

}

