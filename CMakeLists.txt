cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INSTALL_PREFIX .)

find_library(CUVID_LIB nvcuvid)
find_library(NVENCODEAPI_LIB nvidia-encode)

include(CTest)
enable_testing()

find_package(CUDA)

set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_50,code=\"sm_50,compute_50\")

if(CMAKE_COMPILER_IS_GNUCC)
    if(NOT "${CUDA_NVCC_FLAGS}" MATCHES "-std=c\\+\\+11")
        list(APPEND CUDA_NVCC_FLAGS -std=c++11)
    endif()
endif()

include_directories(
    /usr/local/cuda/include
    common/inc
    inc
    /usr/include/freetype2
    nvidia/inc
    nvidia/NvDecoder
    nvidia/NvEncoder
    nvidia/Utils
)

add_executable(main
    nvidia/NvEncoder/NvEncoder.cpp
    nvidia/NvDecoder/NvDecoder.cpp
    nvidia/NvEncoder/NvEncoderCuda.cpp
    
    src/FFmpegDecoder.cpp
    src/FFmpegEncoded.cpp

    common/src/drawing.cpp
    common/src/freetype_helper.cpp
    common/src/utils.cpp

    # src/ffmpeg_push_stream.cpp

    # nvidia/Utils/NvCodecUtils.h
    # nvidia/Utils/Logger.h
    # src/FFmpegEncoded.cpp
    main.cpp
)

target_link_libraries(main
    pthread
    freetype
    avcodec
    avutil
    swscale
    avformat
    ${CUDA_CUDA_LIBRARY}
    ${CMAKE_DL_LIBS}

    ${CMAKE_CURRENT_SOURCE_DIR}/nvidia/lib/libnvcuvid.so
    ${CMAKE_CURRENT_SOURCE_DIR}/nvidia/lib/libnvidia-encode.so
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
