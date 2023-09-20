
#pragma once

#include <iostream>
#include <string>

#include "utils.h"

extern "C"
{
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
};

typedef int (*FrameProcessCallBack)(void *callback_param, void *frame_data,
                                    int frame_size);

class FFmpegDecoder
{
public:
    FFmpegDecoder(const std::string &name);
    ~FFmpegDecoder() {}
    void Decode(FrameProcessCallBack callback_func, void *callback_param);
    int GetFrameWidth()
    {
        return frameWidth_;
    }
    int GetFrameHeight()
    {
        return frameHeight_;
    }
    int GetVideoType()
    {
        return videoType_;
    }
    int GetFps()
    {
        return fps_;
    }
    int IsFinished()
    {
        return isFinished_;
    }
    int GetProfile()
    {
        return profile_;
    }
    void SetTransport(const std::string &transportType);
    void StopDecode()
    {
        isStop_ = true;
    }

private:
    int GetVideoIndex(AVFormatContext *av_format_context);
    void GetVideoInfo();
    void InitVideoStreamFilter(const AVBitStreamFilter *&video_filter);
    bool OpenVideo(AVFormatContext *&av_format_context);
    void SetDictForRtsp(AVDictionary *&avdic);
    bool InitVideoParams(int videoIndex,
                         AVFormatContext *av_format_context,
                         AVBSFContext *&bsf_ctx);

private:
    bool isFinished_;
    bool isStop_;
    int frameWidth_;
    int frameHeight_;
    int videoType_;
    int profile_;
    int fps_;
    std::string streamName_;
    std::string rtspTransport_;
};