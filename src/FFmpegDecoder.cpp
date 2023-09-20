
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <fstream>
#include <memory>
#include <thread>
#include <cstring>
#include <iostream>

#include "FFmpegDecoder.h"

using namespace std;
#define DEVICE_MAX  4

namespace {
    const int64_t kUsec = 1000000;
    const uint32_t kDecodeFrameQueueSize = 256;
    const int kDecodeQueueOpWait = 10000; // decode wait 10ms/frame
    const int kFrameEnQueueRetryTimes = 1000; // max wait time for the frame to enter in queue
    const int kQueueOpRetryTimes = 1000;
    const int kOutputJamWait = 10000;
    const int kInvalidTpye = -1;
    const int kWaitDecodeFinishInterval = 1000;
    const int kDefaultFps = 1;
    const int kReadSlow = 5;
    const uint32_t kVideoChannelMax310 = 32;
    const uint32_t kVideoChannelMax310P = 256;

    // ChannelIdGenerator channelIdGenerator[DEVICE_MAX] = {};

    const int kNoFlag = 0; // no flag
    const int kInvalidVideoIndex = -1; // invalid video index
    const string kRtspTransport = "rtspTransport"; // rtsp transport
    const string kUdp = "udp"; // video format udp
    const string kTcp = "tcp";
    const string kBufferSize = "buffer_size"; // buffer size string
    const string kMaxBufferSize = "10485760"; // maximum buffer size:10MB
    const string kMaxDelayStr = "max_delay"; // maximum delay string
    const string kMaxDelayValue = "100000000"; // maximum delay time:100s
    const string kTimeoutStr = "stimeout"; // timeout string
    const string kTimeoutValue = "5000000"; // timeout:5s
    const string kPktSize = "pkt_size"; // ffmpeg pakect size string
    const string kPktSizeValue = "10485760"; // ffmpeg packet size value:10MB
    const string kReorderQueueSize = "reorder_queue_size"; // reorder queue size
    const string kReorderQueueSizeValue = "0"; // reorder queue size value
    const int kErrorBufferSize = 1024; // buffer size for error info
    const uint32_t kDefaultStreamFps = 5;
    const uint32_t kOneSecUs = 1000 * 1000;
}

FFmpegDecoder::FFmpegDecoder(const std::string& streamName):streamName_(streamName)
{
    rtspTransport_.assign(kTcp.c_str());
    isFinished_ = false;
    isStop_ = false;
    GetVideoInfo();
}

void FFmpegDecoder::SetTransport(const std::string& transportType)
{
    rtspTransport_.assign(transportType.c_str());
};

int FFmpegDecoder::GetVideoIndex(AVFormatContext* avFormatContext)
{
    if (avFormatContext == nullptr) { // verify input pointer
        return kInvalidVideoIndex;
    }

    // get video index in streams
    for (uint32_t i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type
            == AVMEDIA_TYPE_VIDEO) { // check is media type is video
            return i;
        }
    }

    return kInvalidVideoIndex;
}

void FFmpegDecoder::InitVideoStreamFilter(const AVBitStreamFilter*& videoFilter)
{
    if (videoType_ == AV_CODEC_ID_H264) { // check video type is h264
        videoFilter = av_bsf_get_by_name("h264_mp4toannexb");
    } else { // the video type is h265
        videoFilter = av_bsf_get_by_name("hevc_mp4toannexb");
    }
}

void FFmpegDecoder::SetDictForRtsp(AVDictionary*& avdic)
{
    ACLLITE_LOG_INFO("Set parameters for %s", streamName_.c_str());

    av_dict_set(&avdic, kRtspTransport.c_str(), rtspTransport_.c_str(), kNoFlag);
    av_dict_set(&avdic, kBufferSize.c_str(), kMaxBufferSize.c_str(), kNoFlag);
    av_dict_set(&avdic, kMaxDelayStr.c_str(), kMaxDelayValue.c_str(), kNoFlag);
    av_dict_set(&avdic, kTimeoutStr.c_str(), kTimeoutValue.c_str(), kNoFlag);
    av_dict_set(&avdic, kReorderQueueSize.c_str(),
                kReorderQueueSizeValue.c_str(), kNoFlag);
    av_dict_set(&avdic, kPktSize.c_str(), kPktSizeValue.c_str(), kNoFlag);
    ACLLITE_LOG_INFO("Set parameters for %s end", streamName_.c_str());
}

bool FFmpegDecoder::OpenVideo(AVFormatContext*& avFormatContext)
{
    bool ret = true;
    AVDictionary* avdic = nullptr;

    av_log_set_level(AV_LOG_DEBUG);

    ACLLITE_LOG_INFO("Open video %s ...", streamName_.c_str());
    SetDictForRtsp(avdic);
    int openRet = avformat_open_input(&avFormatContext,
                                      streamName_.c_str(), nullptr,
                                      &avdic);
    if (openRet < 0) { // check open video result
        char buf_error[kErrorBufferSize];
        av_strerror(openRet, buf_error, kErrorBufferSize);

        ACLLITE_LOG_ERROR("Could not open video:%s, return :%d, error info:%s",
                          streamName_.c_str(), openRet, buf_error);
        ret = false;
    }

    if (avdic != nullptr) { // free AVDictionary
        av_dict_free(&avdic);
    }

    return ret;
}

bool FFmpegDecoder::InitVideoParams(int videoIndex,
                                    AVFormatContext* avFormatContext,
                                    AVBSFContext*& bsfCtx)
{
    const AVBitStreamFilter* videoFilter = nullptr;
    InitVideoStreamFilter(videoFilter);
    if (videoFilter == nullptr) { // check video fileter is nullptr
        ACLLITE_LOG_ERROR("Unkonw bitstream filter, videoFilter is nullptr!");
        return false;
    }

    // checke alloc bsf context result
    if (av_bsf_alloc(videoFilter, &bsfCtx) < 0) {
        ACLLITE_LOG_ERROR("Fail to call av_bsf_alloc!");
        return false;
    }

    // check copy parameters result
    if (avcodec_parameters_copy(bsfCtx->par_in,
        avFormatContext->streams[videoIndex]->codecpar) < 0) {
        ACLLITE_LOG_ERROR("Fail to call avcodec_parameters_copy!");
        return false;
    }

    bsfCtx->time_base_in = avFormatContext->streams[videoIndex]->time_base;

    // check initialize bsf contextreult
    if (av_bsf_init(bsfCtx) < 0) {
        ACLLITE_LOG_ERROR("Fail to call av_bsf_init!");
        return false;
    }

    return true;
}

void FFmpegDecoder::Decode(FrameProcessCallBack callback,
                           void *callbackParam)
{
    ACLLITE_LOG_INFO("Start ffmpeg decode video %s ...", streamName_.c_str());
    avformat_network_init(); // init network

    AVFormatContext* avFormatContext = avformat_alloc_context();

    // check open video result
    if (!OpenVideo(avFormatContext)) {
        return;
    }

    int videoIndex = GetVideoIndex(avFormatContext);
    if (videoIndex == kInvalidVideoIndex) { // check video index is valid
        ACLLITE_LOG_ERROR("Rtsp %s index is -1", streamName_.c_str());
        return;
    }

    AVBSFContext* bsfCtx = nullptr;
    // check initialize video parameters result
    if (!InitVideoParams(videoIndex, avFormatContext, bsfCtx)) {
        return;
    }

    ACLLITE_LOG_INFO("Start decode frame of video %s ...", streamName_.c_str());

    AVPacket avPacket;
    int processOk = true;
    // loop to get every frame from video stream
    while ((av_read_frame(avFormatContext, &avPacket) == 0) && processOk && !isStop_) {
        if (avPacket.stream_index == videoIndex) { // check current stream is video
          // send video packet to ffmpeg
            if (av_bsf_send_packet(bsfCtx, &avPacket)) {
                ACLLITE_LOG_ERROR("Fail to call av_bsf_send_packet, channel id:%s",
                    streamName_.c_str());
            }

            // receive single frame from ffmpeg
            while ((av_bsf_receive_packet(bsfCtx, &avPacket) == 0) && !isStop_) {
                int ret = callback(callbackParam, avPacket.data, avPacket.size);
                if (ret != 0) {
                    processOk = false;
                    break;
                }
            }
        }
        av_packet_unref(&avPacket);
    }

    av_bsf_free(&bsfCtx); // free AVBSFContext pointer
    avformat_close_input(&avFormatContext); // close input video

    isFinished_ = true;
    ACLLITE_LOG_INFO("Ffmpeg decoder %s finished", streamName_.c_str());
}

void FFmpegDecoder::GetVideoInfo()
{
    avformat_network_init(); // init network
    AVFormatContext* avFormatContext = avformat_alloc_context();
    bool ret = OpenVideo(avFormatContext);
    if (ret == false) {
        ACLLITE_LOG_ERROR("Open %s failed", streamName_.c_str());
        return;
    }

    if (avformat_find_stream_info(avFormatContext, NULL)<0) {
        ACLLITE_LOG_ERROR("Get stream info of %s failed", streamName_.c_str());
        return;
    }

    int videoIndex = GetVideoIndex(avFormatContext);
    if (videoIndex == kInvalidVideoIndex) { // check video index is valid
        ACLLITE_LOG_ERROR("Video index is %d, current media stream has no "
                          "video info:%s",
                          kInvalidVideoIndex, streamName_.c_str());
        avformat_close_input(&avFormatContext);
        return;
    }

    AVStream* inStream = avFormatContext->streams[videoIndex];

    frameWidth_ = inStream->codecpar->width;
    frameHeight_ = inStream->codecpar->height;
    if (inStream->avg_frame_rate.den) {
        fps_ = inStream->avg_frame_rate.num / inStream->avg_frame_rate.den;
    } else {
        fps_ = kDefaultStreamFps;
    }

    videoType_ = inStream->codecpar->codec_id;
    profile_ = inStream->codecpar->profile;

    avformat_close_input(&avFormatContext);

    ACLLITE_LOG_INFO("Video %s, type %d, profile %d, width:%d, height:%d, fps:%d",
                     streamName_.c_str(), videoType_, profile_, frameWidth_, frameHeight_, fps_);
    return;
}