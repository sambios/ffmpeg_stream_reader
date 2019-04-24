cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 14)
project(ffmpeg_rtsp_reader)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(ffmpeg_rtsp_reader main.cpp netstream_reader.cpp
        netstream_reader_ffmpeg.cpp)
target_link_libraries(ffmpeg_rtsp_reader avcodec avformat avutil swscale)
