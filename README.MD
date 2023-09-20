
ffmpeg拉的裸流数据 
src/FFmpegDecoder.cpp

ffmpeg推编码好的裸流数据
src/FFmpegEncoded.cpp

拉流裸流包->(void *data,int size)->硬件解码->(void *yuvdata,int w,int h)->绘制->(void *data, int size)硬编码-> 推流