#include <iostream>
#include "FFmpegDecoder.h"

/// <summary>
/// 帧转图片
/// 如果外部提供的缓存长度不足则不会写入。
/// </summary>
/// <param name="frame">[in]视频帧</param>
/// <param name="codecID">[in]图片编码器ID,如jpg:AV_CODEC_ID_MJPEG，png:AV_CODEC_ID_PNG</param>
/// <param name="outbuf">[out]图片缓存，由外部提供</param>
/// <param name="outbufSize">[in]图片缓存长度</param>
/// <returns>返回图片实际长度</returns>
static int frameToImage(AVFrame* frame, enum AVCodecID codecID, uint8_t* outbuf, size_t outbufSize)
{
	int ret = 0;
	AVPacket pkt;
	AVCodec* codec;
	AVCodecContext* ctx = NULL;
	AVFrame* rgbFrame = NULL;
	uint8_t* buffer = NULL;
	struct SwsContext* swsContext = NULL;
	av_init_packet(&pkt);
	codec = avcodec_find_encoder(codecID);
	if (!codec)
	{
		printf("avcodec_send_frame error %d", codecID);
		goto end;
	}
	if (!codec->pix_fmts)
	{
		printf("unsupport pix format with codec %s", codec->name);
		goto end;
	}
	ctx = avcodec_alloc_context3(codec);
	ctx->bit_rate = 3000000;
	ctx->width = frame->width;
	ctx->height = frame->height;
	ctx->time_base.num = 1;
	ctx->time_base.den = 25;
	ctx->gop_size = 10;
	ctx->max_b_frames = 0;
	ctx->thread_count = 1;
	ctx->pix_fmt = *codec->pix_fmts;
	ret = avcodec_open2(ctx, codec, NULL);
	if (ret < 0)
	{
		printf("avcodec_open2 error %d", ret);
		goto end;
	}
	if (frame->format != ctx->pix_fmt)
	{
		rgbFrame = av_frame_alloc();
		if (rgbFrame == NULL)
		{
			printf("av_frame_alloc  fail");
			goto end;
		}
		swsContext = sws_getContext(frame->width, frame->height, (enum AVPixelFormat)frame->format, frame->width, frame->height, ctx->pix_fmt, 1, NULL, NULL, NULL);
		if (!swsContext)
		{
			printf("sws_getContext  fail");
			goto end;
		}
		int bufferSize = av_image_get_buffer_size(ctx->pix_fmt, frame->width, frame->height, 1) * 2;
		buffer = (unsigned char*)av_malloc(bufferSize);
		if (buffer == NULL)
		{
			printf("buffer alloc fail:%d", bufferSize);
			goto end;
		}
		av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, ctx->pix_fmt, frame->width, frame->height, 1);
		if ((ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize)) < 0)
		{
			printf("sws_scale error %d", ret);
		}
		rgbFrame->format = ctx->pix_fmt;
		rgbFrame->width = ctx->width;
		rgbFrame->height = ctx->height;
		ret = avcodec_send_frame(ctx, rgbFrame);
	}
	else
	{
		ret = avcodec_send_frame(ctx, frame);
	}
	if (ret < 0)
	{
		printf("avcodec_send_frame error %d", ret);
		goto end;
	}
	ret = avcodec_receive_packet(ctx, &pkt);
	if (ret < 0)
	{
		printf("avcodec_receive_packet error %d", ret);
		goto end;
	}
	if (pkt.size > 0 && pkt.size <= outbufSize)
		memcpy(outbuf, pkt.data, pkt.size);
	ret = pkt.size;
end:
	if (swsContext)
	{
		sws_freeContext(swsContext);
	}
	if (rgbFrame)
	{
		av_frame_unref(rgbFrame);
		av_frame_free(&rgbFrame);
	}
	if (buffer)
	{
		av_free(buffer);
	}
	av_packet_unref(&pkt);
	if (ctx)
	{
		avcodec_close(ctx);
		avcodec_free_context(&ctx);
	}
	return ret;
}

/// <summary>
/// 将视频帧保存为jpg图片
/// </summary>
/// <param name="frame">视频帧</param>
/// <param name="path">保存的路径</param>
void saveFrameToJpg(AVFrame*frame,const char*path) {
	//确保缓冲区长度大于图片,使用brga像素格式计算。如果是bmp或tiff依然可能超出长度，需要加一个头部长度，或直接乘以2。
	int bufSize = av_image_get_buffer_size(AV_PIX_FMT_BGRA, frame->width, frame->height, 64);
	//申请缓冲区
	uint8_t* buf = (uint8_t*)av_malloc(bufSize);
	//将视频帧转换成jpg图片，如果需要png则使用AV_CODEC_ID_PNG
	int picSize = frameToImage(frame, AV_CODEC_ID_MJPEG, buf, bufSize);
	//写入文件
	auto f = fopen(path, "wb+");
	if (f)
	{
		fwrite(buf, sizeof(uint8_t), bufSize, f);
		fclose(f);
	}
	//释放缓冲区
	av_free(buf);
}

/// <summary>
/// 通过裸数据生成avframe
/// </summary>
/// <param name="frameData">帧数据</param>
/// <param name="width">帧宽</param>
/// <param name="height">帧高</param>
/// <param name="format">像素格式</param>
/// <returns>avframe，使用完成后需要调用av_frame_free释放</returns>
AVFrame* allocFrame(uint8_t*frameData,int width,int height,AVPixelFormat format) {
	AVFrame* frame = av_frame_alloc();
	frame->width = width;
	frame->height = height;
	frame->format = format;
	av_image_fill_arrays(frame->data, frame->linesize, frameData, format, frame->width, frame->height, 64);
	return frame;
}


int FFmpegDecoderFrameProcessCallBack(void *callback_param, void *frame_data,
                                      int frame_size)
{
    // printf("frame data:[%d]", frame_size);
    fprintf(stdout,"data[%d]\n", frame_size);
    uint8_t*frameData = (uint8_t*)frame_data;
    
	AVFrame* frame=allocFrame(frameData,1280,720,AV_PIX_FMT_YUV420P);
	saveFrameToJpg(frame,"snapshot.jpg");//此方法定义在示例1中
	av_frame_free(&frame);

    return 0;
}
int main(int, char **)
{
    FFmpegDecoder decoder = FFmpegDecoder("rtsp://admin:ad123456@192.168.137.50:554/h264");
    decoder.Decode(FFmpegDecoderFrameProcessCallBack, NULL);
    
}
