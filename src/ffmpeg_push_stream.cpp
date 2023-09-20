
namespace
{

}

#include <iostream>
#include <string>
#include <fstream>
#include <thread>

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

#include <chrono>
#include <string>
#include <tuple>

using Duration = std::chrono::duration<double, std::ratio<1>>;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

class FFmpegPushStream
{
private:
    AVFormatContext *avFormatContext_{nullptr};
    AVCodecContext *avCodecContext_{nullptr};
    AVStream *g_avStream{nullptr};
    AVCodec *avCodec_{nullptr};

    AVPacket *avPacket_{nullptr};

    AVFrame *g_yuvFrame{nullptr};
    uint8_t *g_yuvBuf{nullptr};
    bool g_yuvToRtspFlag;

    AVFrame *g_brgFrame{nullptr};
    uint8_t *g_brgBuf{nullptr};
    bool g_bgrToRtspFlag;

    int g_yuvSize;
    int g_rgbSize;

    struct SwsContext *swsContext_;

    AVFrame *video_frame{nullptr};
    AVDictionary *codec_options{nullptr};

    std::string stream_name;
    int h;
    int w;

    Duration interval;
    TimePoint last_sent_tp;

    bool output_is_file;

    bool valid{false};

public:
    FFmpegPushStream(/* args */);
    ~FFmpegPushStream();

    int Init(std::string name, int img_h, int img_w, int frame_rate, int pic_fmt);
    int Init(std::string name, int img_h, int img_w, AVRational frame_rate, int pic_fmt);

    void YuvDataInit();
    void BgrDataInint();

    int WriteYuvData(void *dataBuf, uint32_t size, uint32_t seq);

    int WriteBgrData(void *dataBuf, uint32_t size, uint32_t seq);

    void Wait4Stream();
    int WriteFrameData(void *data, int size);
};

FFmpegPushStream::FFmpegPushStream(/* args */)
{
}

static std::string GuessFormatFromName(const std::string &name)
{
    std::string format;
    if (name.find("rtmp:") == 0)
    {
        format = "flv";
    }
    else if (name.find("rtsp:") == 0)
    {
        format = "rtsp";
    }
    return format;
}

int FFmpegPushStream::Init(std::string name, int img_h, int img_w, int frame_rate,
                           int pic_fmt)
{
    AVRational av_framerate;
    av_framerate.num = frame_rate;
    av_framerate.den = 1;
    return Init(name, img_h, img_w, av_framerate, pic_fmt);
}

int FFmpegPushStream::Init(std::string name, int img_h, int img_w,
                           AVRational frame_rate, int pic_fmt)
{
    last_sent_tp = std::chrono::steady_clock::now();
    interval = Duration((double)frame_rate.den / frame_rate.num);
    std::cout << "FFMPEGOutput::Init frame send interval: " << interval.count()
              << std::endl;
    stream_name = name;
    const char *output = stream_name.c_str();
    const char *profile = "high444";

    std::string format = GuessFormatFromName(name);

    av_register_all();
    
    avformat_network_init();
    // av_log_set_level(AV_LOG_TRACE);
    int ret = 0;

    avFormatContext_ = NULL;
    ret = avformat_alloc_output_context2(
        &avFormatContext_, NULL, format.empty() ? NULL : format.c_str(), output);
    if (ret < 0)
    {
        std::cerr << "[FFMPEGOutput::Init] avformat_alloc_output_context2 failed"
                  << ret << std::endl;
        return ret;
    }

    // avFormatContext_->flags |= AVFMT_FLAG_NOBUFFER;
    avFormatContext_->flags |= AVFMT_FLAG_FLUSH_PACKETS;

    if (!(avFormatContext_->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open2(&avFormatContext_->pb, output, AVIO_FLAG_WRITE, NULL, NULL);
        if (ret < 0)
        {
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            std::cerr << "[FFMPEGOutput::Init] avio_open2 failed err code: " << ret
                      << " Reason: "
                      << av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret)
                      << std::endl;
            return ret;
        }
    }

    avCodec_ = avcodec_find_encoder(AV_CODEC_ID_H264);

    avFormatContext_->video_codec = avCodec_;
    avFormatContext_->video_codec_id = AV_CODEC_ID_H264;

    avCodecContext_ = avcodec_alloc_context3(avCodec_);

    avCodecContext_->codec_tag = 0;
    avCodecContext_->codec_id = AV_CODEC_ID_H264;
    avCodecContext_->codec_type = AVMEDIA_TYPE_VIDEO;
    avCodecContext_->gop_size = 12;
    avCodecContext_->height = img_h;
    avCodecContext_->width = img_w;
    avCodecContext_->pix_fmt =
        (AVPixelFormat)pic_fmt; // AV_PIX_FMT_NV12;// NV12 IS YUV420
    // control rate
    avCodecContext_->bit_rate = 0;
    avCodecContext_->rc_buffer_size = 0;
    avCodecContext_->rc_max_rate = 0;
    avCodecContext_->rc_min_rate = 0;
    avCodecContext_->time_base.num = frame_rate.den;
    avCodecContext_->time_base.den = frame_rate.num;
    // avCodecContext_->gop_size = 0;

    if (avFormatContext_->oformat->flags & AVFMT_GLOBALHEADER)
    {
        avCodecContext_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    g_avStream = avformat_new_stream(avFormatContext_, avCodec_);

    ret = avcodec_parameters_from_context(g_avStream->codecpar, avCodecContext_);

    if (ret < 0)
    {
        std::cerr << "[FFMPEGOutput::Init] avcodec_parameters_from_context failed"
                  << std::endl;
        return ret;
    }

    codec_options = NULL;
    av_dict_set(&codec_options, "profile", profile, 0);
    av_dict_set(&codec_options, "preset", "superfast", 0);
    av_dict_set(&codec_options, "tune", "zerolatency", 0);

    ret = avcodec_open2(avCodecContext_, avCodec_, &codec_options);
    if (ret < 0)
    {
        std::cerr << "[FFMPEGOutput::Init] avformat_new_stream failed" << std::endl;
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        std::cerr << "[FFMPEGInput::Init] err string: "
                  << av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret)
                  << std::endl;
        return ret;
    }

    g_avStream->codecpar->extradata = avCodecContext_->extradata;
    g_avStream->codecpar->extradata_size = avCodecContext_->extradata_size;

    av_dump_format(avFormatContext_, 0, output, 1);

    ret = avformat_write_header(avFormatContext_, NULL);
    if (ret < 0)
    {
        std::cerr << "[FFMPEGOutput::init] avformat_write_header failed"
                  << std::endl;
        return ret;
    }

    video_frame = av_frame_alloc();
    int frame_buf_size = av_image_get_buffer_size(
        avCodecContext_->pix_fmt, avCodecContext_->width, avCodecContext_->height, 1);
    std::cout << "[FFMPEGOutput::Init] expected frame size " << frame_buf_size
              << std::endl;

    video_frame->width = avCodecContext_->width;
    video_frame->height = avCodecContext_->height;
    video_frame->format = avCodecContext_->pix_fmt;
    video_frame->pts = 1;

    valid = true;
    std::ifstream test_f(name.c_str());
    output_is_file = test_f.good();
}

FFmpegPushStream::~FFmpegPushStream()
{
}

void FFmpegPushStream::YuvDataInit()
{
    if (this->g_yuvToRtspFlag == false)
    {
        g_yuvFrame = av_frame_alloc();
        g_yuvFrame->width = avCodecContext_->width;
        g_yuvFrame->height = avCodecContext_->height;
        g_yuvFrame->format = avCodecContext_->pix_fmt;

        g_yuvSize = av_image_get_buffer_size(avCodecContext_->pix_fmt, avCodecContext_->width, avCodecContext_->height, 1);

        g_yuvBuf = (uint8_t *)av_malloc(g_yuvSize);

        int ret = av_image_fill_arrays(g_yuvFrame->data, g_yuvFrame->linesize,
                                       g_yuvBuf, avCodecContext_->pix_fmt,
                                       avCodecContext_->width, avCodecContext_->height, 1);
        this->g_yuvToRtspFlag = true;
    }
}

int FFmpegPushStream::WriteYuvData(void *dataBuf, uint32_t size, uint32_t seq)
{
    memcpy(g_yuvBuf, dataBuf, size);
    g_yuvFrame->pts = seq;
    if (avcodec_send_frame(avCodecContext_, g_yuvFrame) >= 0)
    {
        while (avcodec_receive_packet(avCodecContext_, avPacket_) >= 0)
        {
            avPacket_->stream_index = g_avStream->index;
            av_packet_rescale_ts(avPacket_, avCodecContext_->time_base, g_avStream->time_base);
            avPacket_->pos = -1;
            int ret = av_interleaved_write_frame(avFormatContext_, avPacket_);
            if (ret < 0)
            {
                ACLLITE_LOG_ERROR("error is: %d", ret);
            }
        }
    }
    return ACLLITE_OK;
}

void FFmpegPushStream::BgrDataInint()
{
    if (this->g_bgrToRtspFlag == false)
    {
        g_brgFrame = av_frame_alloc();
        g_yuvFrame = av_frame_alloc();
        g_brgFrame->width = avCodecContext_->width;
        g_yuvFrame->width = avCodecContext_->width;
        g_brgFrame->height = avCodecContext_->height;
        g_yuvFrame->height = avCodecContext_->height;
        g_brgFrame->format = AV_PIX_FMT_BGR24;
        g_yuvFrame->format = avCodecContext_->pix_fmt;

        g_rgbSize = av_image_get_buffer_size(AV_PIX_FMT_BGR24, avCodecContext_->width, avCodecContext_->height, 1);
        g_yuvSize = av_image_get_buffer_size(avCodecContext_->pix_fmt, avCodecContext_->width, avCodecContext_->height, 1);

        g_brgBuf = (uint8_t *)av_malloc(g_rgbSize);
        g_yuvBuf = (uint8_t *)av_malloc(g_yuvSize);

        int ret = av_image_fill_arrays(g_brgFrame->data, g_brgFrame->linesize,
                                       g_brgBuf, AV_PIX_FMT_BGR24,
                                       avCodecContext_->width, avCodecContext_->height, 1);

        ret = av_image_fill_arrays(g_yuvFrame->data, g_yuvFrame->linesize,
                                   g_yuvBuf, avCodecContext_->pix_fmt,
                                   avCodecContext_->width, avCodecContext_->height, 1);
        swsContext_ = sws_getContext(
            avCodecContext_->width, avCodecContext_->height, AV_PIX_FMT_BGR24,
            avCodecContext_->width, avCodecContext_->height, avCodecContext_->pix_fmt,
            SWS_BILINEAR, NULL, NULL, NULL);
        this->g_bgrToRtspFlag = true;
    }
}

int FFmpegPushStream::WriteBgrData(void *dataBuf, uint32_t size, uint32_t seq)
{
    if (g_rgbSize != size)
    {
        ACLLITE_LOG_ERROR("bgr data size error, The data size should be %d, but the actual size is %d", g_rgbSize, size);
        return -1;
    }
    memcpy(g_brgBuf, dataBuf, g_rgbSize);
    sws_scale(swsContext_,
              g_brgFrame->data,
              g_brgFrame->linesize,
              0,
              avCodecContext_->height,
              g_yuvFrame->data,
              g_yuvFrame->linesize);
    g_yuvFrame->pts = seq;
    if (avcodec_send_frame(avCodecContext_, g_yuvFrame) >= 0)
    {
        while (avcodec_receive_packet(avCodecContext_, avPacket_) >= 0)
        {
            avPacket_->stream_index = g_avStream->index;
            av_packet_rescale_ts(avPacket_, avCodecContext_->time_base, g_avStream->time_base);
            avPacket_->pos = -1;
            int ret = av_interleaved_write_frame(avFormatContext_, avPacket_);
            if (ret < 0)
            {
                ACLLITE_LOG_ERROR("error is: %d", ret);
            }
        }
    }
    return ACLLITE_OK;
}

void FFmpegPushStream::Wait4Stream()
{
    if (!output_is_file)
    {
        // if output is not file, e.g. RTSP or RTMP
        // we must not send too fast
        auto now = std::chrono::steady_clock::now();
        auto dt = now - last_sent_tp;
        auto dt_us = std::chrono::duration_cast<std::chrono::microseconds>(dt);
        auto dt_sec = std::chrono::duration_cast<Duration>(dt_us);
        auto time_to_sleep = interval - dt_sec;
        if (time_to_sleep.count() > 0)
        {
            std::this_thread::sleep_for(time_to_sleep);
        }
        last_sent_tp = std::chrono::steady_clock::now();
    }
}

static void custom_free(void *opaque, uint8_t *data)
{
    free(data);
}

int FFmpegPushStream::WriteFrameData(void *pdata, int size)
{
    Wait4Stream();
    //   APP_PROFILE(FFMPEGOutput::SendEncodedFrame);
    int ret = 0;
    AVPacket pkt = {0};
    av_init_packet(&pkt);

    pkt.pts = video_frame->pts;
    pkt.dts = pkt.pts;
    pkt.flags = AV_PKT_FLAG_KEY;
    // av_packet_from_data(&pkt, (uint8_t*)pdata, size);

    pkt.buf = av_buffer_create((uint8_t *)pdata, size, custom_free, NULL, 0);

    pkt.data = (uint8_t *)pdata;
    pkt.size = size;

    ret = av_write_frame(avFormatContext_, &pkt);

    if (ret < 0)
    {
        std::cerr << "[FFMPEGOutput::SendFrame] av_interleaved_write_frame failed"
                  << std::endl;
        return 0;
    }

    // pkt.buf = nullptr;

    av_packet_unref(&pkt);

    video_frame->pts += av_rescale_q(1, avCodecContext_->time_base, g_avStream->time_base);
}