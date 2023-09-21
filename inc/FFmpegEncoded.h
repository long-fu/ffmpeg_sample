

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
}

#include <chrono>
#include <string>
#include <tuple>

using Duration = std::chrono::duration<double, std::ratio<1>>;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

class FFmpegEncoder {
public:
  FFmpegEncoder() = default;

  int Init(std::string name, int img_h, int img_w, int frame_rate = 20,
           int pic_fmt = AV_PIX_FMT_NV12);
  int Init(std::string name, int img_h, int img_w, AVRational frame_rate,
           int pic_fmt = AV_PIX_FMT_NV12);
  bool IsValid();

  void ShutDown() {}
  void Process(const uint8_t *pdata);
  void Process(void *pdata, int size);

  void Wait4Stream();

  void SendFrame(const uint8_t *pdata);
  void SendEncodedFrame(void *pdata, int size);
  void Close();

private:
  AVFormatContext *encoder_avfc{nullptr};
  AVCodec *video_avc{nullptr};
  AVCodecContext *video_avcc{nullptr};
  AVStream *avs{nullptr};
  AVDictionary *codec_options{nullptr};
  AVFrame *video_frame{nullptr};

  std::string stream_name;
  int h;
  int w;

  Duration interval;
  TimePoint last_sent_tp;

  bool output_is_file;

  bool valid{false};
};


