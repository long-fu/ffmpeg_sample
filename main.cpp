#include <iostream>
#include "FFmpegDecoder.h"
#include "NvDecoder.h"
#include <cuda.h>
#include "AppDecUtils.h"
#include "NvEncoderCLIOptions.h"
#include "NvEncoderCuda.h"
#include "FFmpegEncoded.h"
simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

typedef int (*NvEncodeCallBack)(void *callback_param, uint8_t *packet, int packet_size);

typedef struct _UserData
{
    NvDecoder *dec;
    NvEncoderCuda *enc;
    FFmpegEncoder *encoder;
    CUcontext cuContext1;
} UserData;

void EncodeProc(CUdevice cuDevice,
                int nWidth, int nHeight,
                NV_ENC_BUFFER_FORMAT eFormat, NvEncoderInitParam *pEncodeCLIOptions,
                bool bBgra64,
                // const char *szInFilePath,
                void *frame_data, int frame_size,
                std::exception_ptr &encExceptionPtr,
                void *userData, NvEncodeCallBack callBack)
{
    CUdeviceptr dpFrame = 0, dpBgraFrame = 0;
    CUcontext cuContext = NULL;

    try
    {
        ck(cuCtxCreate(&cuContext, 0, cuDevice));

        NvEncoderCuda enc(cuContext, nWidth, nHeight, eFormat, 3, false, false, false);
        NV_ENC_INITIALIZE_PARAMS initializeParams = {NV_ENC_INITIALIZE_PARAMS_VER};
        NV_ENC_CONFIG encodeConfig = {NV_ENC_CONFIG_VER};
        initializeParams.encodeConfig = &encodeConfig;

        enc.CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_P3_GUID, NV_ENC_TUNING_INFO_HIGH_QUALITY);

        initializeParams.bufferFormat = eFormat;
        // pEncodeCLIOptions->SetInitParams(&initializeParams, eFormat);

        enc.CreateEncoder(&initializeParams);

        // std::ifstream fpIn(szInFilePath, std::ifstream::in | std::ifstream::binary);
        // if (!fpIn)
        // {
        //     std::cout << "Unable to open input file: " << szInFilePath << std::endl;
        //     return;
        // }
        std::cout << "GetFrameSize" << nWidth * nHeight * 8 << " " << enc.GetFrameSize() << std::endl;
        // int nHostFrameSize = bBgra64 ? nWidth * nHeight * 8 : enc.GetFrameSize();
        // std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nHostFrameSize]);
        void *pHostFrame = frame_data;
        int nHostFrameSize = frame_size;
        CUdeviceptr dpBgraFrame = 0;
        ck(cuMemAlloc(&dpBgraFrame, nWidth * nHeight * 8));
        int nFrame = 0;
        std::streamsize nRead = 0;
        // FFmpegStreamer streamer(pEncodeCLIOptions->IsCodecH264() ? AV_CODEC_ID_H264 : pEncodeCLIOptions->IsCodecHEVC() ? AV_CODEC_ID_HEVC : AV_CODEC_ID_AV1, nWidth, nHeight, 25, szMediaPath);
        do
        {
            std::vector<std::vector<uint8_t>> vPacket;
            // nRead = fpIn.read(reinterpret_cast<char *>(pHostFrame.get()), nHostFrameSize).gcount();
            if (nRead == nHostFrameSize)
            {
                const NvEncInputFrame *encoderInputFrame = enc.GetNextInputFrame();

                if (bBgra64)
                {
                    // Color space conversion
                    ck(cuMemcpyHtoD(dpBgraFrame, pHostFrame, nHostFrameSize));
                    // Bgra64ToP016((uint8_t *)dpBgraFrame, nWidth * 8, (uint8_t *)encoderInputFrame->inputPtr, encoderInputFrame->pitch, nWidth, nHeight);
                }
                else
                {
                    NvEncoderCuda::CopyToDeviceFrame(cuContext, pHostFrame, 0, (CUdeviceptr)encoderInputFrame->inputPtr,
                                                     (int)encoderInputFrame->pitch,
                                                     enc.GetEncodeWidth(),
                                                     enc.GetEncodeHeight(),
                                                     CU_MEMORYTYPE_HOST,
                                                     encoderInputFrame->bufferFormat,
                                                     encoderInputFrame->chromaOffsets,
                                                     encoderInputFrame->numChromaPlanes);
                }
                enc.EncodeFrame(vPacket);
            }
            else
            {
                enc.EndEncode(vPacket);
            }
            for (std::vector<uint8_t> &packet : vPacket)
            {
                packet.data();
                packet.size();
                callBack(userData, packet.data(), (int)packet.size());
            }
        } while (nRead == nHostFrameSize);
        ck(cuMemFree(dpBgraFrame));
        dpBgraFrame = 0;

        enc.DestroyEncoder();
        // fpIn.close();

        std::cout << std::flush << "Total frames encoded: " << nFrame << std::endl
                  << std::flush;
    }
    catch (const std::exception &)
    {
        encExceptionPtr = std::current_exception();
        ck(cuMemFree(dpBgraFrame));
        dpBgraFrame = 0;
        ck(cuMemFree(dpFrame));
        dpFrame = 0;
    }
}

int FFmpegDecoderFrameProcessCallBack(void *callback_param, void *frame_data,
                                      int frame_size)
{
    static int index = 0;
    std::string szOutFilePath = "../out/out_" + std::to_string(index) + ".yuv";
    index++;
    std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
    
    UserData *user_data = (UserData *) callback_param;
    NvDecoder *dec = user_data->dec;
    NvEncoder *enc = user_data->enc;
    CUcontext cuContext = user_data->cuContext1;
    FFmpegEncoder *encder = user_data->encoder;

    uint8_t *pVideo = (uint8_t *)frame_data, *pFrame;
    int nVideoBytes = frame_size, nFrameReturned = 0, nFrame = 0;
    nFrameReturned = dec->Decode(pVideo, nVideoBytes);
    bool bDecodeOutSemiPlanar = false;
    if (!nFrame && nFrameReturned)
        std::cout << dec->GetVideoInfo();

    std::cout << "format: " << dec->GetOutputFormat() << std::endl;
    bDecodeOutSemiPlanar = (dec->GetOutputFormat() == cudaVideoSurfaceFormat_NV12) || (dec->GetOutputFormat() == cudaVideoSurfaceFormat_P016);

    for (int i = 0; i < nFrameReturned; i++)
    {
        pFrame = dec->GetFrame();
        // if (bOutPlanar && bDecodeOutSemiPlanar) {
        //     ConvertSemiplanarToPlanar(pFrame, dec->GetWidth(), dec->GetHeight(), dec->GetBitDepth());
        // }
        // dump YUV to disk
        std::cout << "des: " << dec->GetWidth() << " === " << dec->GetDecodeWidth() << std::endl;
        if (dec->GetWidth() == dec->GetDecodeWidth())
        {
            std::vector<std::vector<uint8_t>> vPacket;
            // fpOut.write(reinterpret_cast<char *>(pFrame), dec->GetFrameSize());
            std::cout << "GetFrameSize " << dec->GetFrameSize() << " " << enc->GetFrameSize() << std::endl;
            const NvEncInputFrame *encoderInputFrame = enc->GetNextInputFrame();
            NvEncoderCuda::CopyToDeviceFrame(cuContext, pFrame, 0, (CUdeviceptr)encoderInputFrame->inputPtr,
                                                                (int)encoderInputFrame->pitch,
                                                                enc->GetEncodeWidth(),
                                                                enc->GetEncodeHeight(),
                                                                CU_MEMORYTYPE_HOST,
                                                                encoderInputFrame->bufferFormat,
                                                                encoderInputFrame->chromaOffsets,
                                                                encoderInputFrame->numChromaPlanes);
            enc->EncodeFrame(vPacket);

            for (std::vector<uint8_t> &packet : vPacket)
            {
                // packet.data();
                // packet.size();
                std::cout << "packet: " << packet.size() ;
                encder->Process(packet.data(),packet.size());
                // callBack(userData, packet.data(), (int)packet.size());
            }
            std::cout << std::endl;
        }
        else
        {
            // 需要进行字节填充
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

    UserData userData;

    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    if (iGpu < 0 || iGpu >= nGpu)
    {
        std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
        return 1;
    }
    CUdevice cuDevice = 0;
    ck(cuDeviceGet(&cuDevice, iGpu));
    char szDeviceName[80];
    ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
    std::cout << "GPU in use: " << szDeviceName << std::endl;

    CUcontext cuContext = NULL;
    createCudaContext(&cuContext, iGpu, 0);

    NvDecoder dec(cuContext, false, cudaVideoCodec_H264, false, false, &cropRect, &resizeDim, bExtractUserSEIMessage);
    /* Set operating point for AV1 SVC. It has no impact for other profiles or codecs
     * PFNVIDOPPOINTCALLBACK Callback from video parser will pick operating point set to NvDecoder  */
    dec.SetOperatingPoint(0, false);

    FFmpegDecoder decoder = FFmpegDecoder("rtsp://admin:ad123456@192.168.137.50:554/h264");

    FFmpegEncoder encoder;
    encoder.Init("rtmp://192.168.2.4:1935/live", 720, 1280, 25, AV_PIX_FMT_NV12);
    
    CUcontext cuContext1 = NULL;
    ck(cuCtxCreate(&cuContext1, 0, cuDevice));

    NvEncoderCuda enc(cuContext1, 1280, 720, NV_ENC_BUFFER_FORMAT_NV12, 3, false, false, false);
    NV_ENC_INITIALIZE_PARAMS initializeParams = {NV_ENC_INITIALIZE_PARAMS_VER};
    NV_ENC_CONFIG encodeConfig = {NV_ENC_CONFIG_VER};
    initializeParams.encodeConfig = &encodeConfig;

    enc.CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_P3_GUID, NV_ENC_TUNING_INFO_HIGH_QUALITY);

    initializeParams.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

    enc.CreateEncoder(&initializeParams);

    userData.dec = &dec;
    userData.enc = &enc;
    userData.encoder = &encoder;
    userData.cuContext1 = cuContext1;
    decoder.Decode(FFmpegDecoderFrameProcessCallBack, &userData);
}
