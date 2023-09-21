#include <iostream>
#include "FFmpegDecoder.h"
#include "NvDecoder.h"
#include <cuda.h>
#include "AppDecUtils.h"


simplelogger::Logger *logger;

int FFmpegDecoderFrameProcessCallBack(void *callback_param, void *frame_data,
                                      int frame_size)
{
    std::string szOutFilePath = "out.yuv";
    std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
    NvDecoder *dec = (NvDecoder *)callback_param;
    uint8_t *pVideo = (uint8_t *)frame_data, *pFrame;
    int nVideoBytes = frame_size, nFrameReturned = 0, nFrame = 0;
    nFrameReturned = dec->Decode(pVideo, nVideoBytes);
    bool bDecodeOutSemiPlanar = false;
    if (!nFrame && nFrameReturned)
        std::cout << dec->GetVideoInfo();
    bDecodeOutSemiPlanar = (dec->GetOutputFormat() == cudaVideoSurfaceFormat_NV12) || (dec->GetOutputFormat() == cudaVideoSurfaceFormat_P016);

    for (int i = 0; i < nFrameReturned; i++)
    {
        pFrame = dec->GetFrame();
        // if (bOutPlanar && bDecodeOutSemiPlanar) {
        //     ConvertSemiplanarToPlanar(pFrame, dec->GetWidth(), dec->GetHeight(), dec->GetBitDepth());
        // }
        // dump YUV to disk
        if (dec->GetWidth() == dec->GetDecodeWidth())
        {
            fpOut.write(reinterpret_cast<char *>(pFrame), dec->GetFrameSize());
        }
        else
        {
            // 4:2:0 output width is 2 byte aligned. If decoded width is odd , luma has 1 pixel padding
            // Remove padding from luma while dumping it to disk
            // dump luma
            for (auto i = 0; i < dec->GetHeight(); i++)
            {
                fpOut.write(reinterpret_cast<char *>(pFrame), dec->GetDecodeWidth() * dec->GetBPP());
                pFrame += dec->GetWidth() * dec->GetBPP();
            }
            // dump Chroma
            fpOut.write(reinterpret_cast<char *>(pFrame), dec->GetChromaPlaneSize());
        }
    }
    nFrame += nFrameReturned;

    return 0;
}
int main(int, char **)
{
    bool bOutPlanar = false;
    int iGpu = 0;
    Rect cropRect = {};
    Dim resizeDim = {};
    unsigned int opPoint = 0;
    bool bDispAllLayers = false;
    bool bExtractUserSEIMessage = false;
    cuInit(0);

    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    if (iGpu < 0 || iGpu >= nGpu)
    {
        std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
        return 1;
    }

    CUcontext cuContext = NULL;
    createCudaContext(&cuContext, iGpu, 0);

    NvDecoder dec(cuContext, false, cudaVideoCodec_H264, false, false, &cropRect, &resizeDim, bExtractUserSEIMessage);
    /* Set operating point for AV1 SVC. It has no impact for other profiles or codecs
     * PFNVIDOPPOINTCALLBACK Callback from video parser will pick operating point set to NvDecoder  */
    dec.SetOperatingPoint(0, false);
    FFmpegDecoder decoder = FFmpegDecoder("rtsp://admin:ad123456@192.168.137.50:554/h264");
    decoder.Decode(FFmpegDecoderFrameProcessCallBack, &dec);
}
