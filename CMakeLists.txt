cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

include_directories(
    common/inc/
    inc/
    /usr/include/freetype2
)

add_executable(main
    common/src/drawing.cpp
    common/src/freetype_helper.cpp
    common/src/utils.cpp
    src/ffmpeg_push_stream.cpp
    src/FFmpegDecoder.cpp
    src/FFmpegEncoded.cpp
    main.cpp
)

target_link_libraries(main pthread freetype avcodec avutil swscale avformat)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
