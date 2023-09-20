#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <unistd.h>
#include <string>
#include <map>

#include "error.h"
#include "type.h"

/**
 * @brief calculate RGB 24bits image size
 * @param [in]: width:  image width
 * @param [in]: height: image height
 * @return bytes size of image
 */
#define RGBU8_IMAGE_SIZE(width, height) ((width) * (height) * 3)

/**
 * @brief calculate RGB C3F32 image size
 * @param [in]: width:  image width
 * @param [in]: height: image height
 * @return bytes size of image
 */
#define RGBF32_IMAGE_SIZE(width, height) ((width) * (height) * 3 * sizeof(float))

/**
 * @brief calculate YUVSP420 image size
 * @param [in]: width:  image width
 * @param [in]: height: image height
 * @return bytes size of image
 */
#define YUV420SP_SIZE(width, height) ((width) * (height) * 3 / 2)

/**
 * @brief calculate YUVSP420 nv12 load to opencv mat height paramter
 * @param [in]: height: yuv image height
 * @return bytes size of image
 */
#define YUV420SP_CV_MAT_HEIGHT(height) ((height) * 3 / 2)

/**
 * @brief generate shared pointer of dvpp memory
 * @param [in]: buf: memory pointer, malloc by acldvppMalloc
 * @return shared pointer of input buffer
 */
// #define SHARED_PTR_DVPP_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { acldvppFree(p); }))

/**
 * @brief generate shared pointer of device memory
 * @param [in]: buf: memory pointer, malloc by acldvppMalloc
 * @return shared pointer of input buffer
 */
// #define SHARED_PTR_DEV_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { aclrtFree(p); }))

/**
 * @brief generate shared pointer of memory
 * @param [in]: buf memory pointer, malloc by new
 * @return shared pointer of input buffer
 */
#define SHARED_PTR_U8_BUF(buf) (shared_ptr<uint8_t>((uint8_t *)(buf), [](uint8_t* p) { delete[](p); }))

/**
 * @brief calculate aligned number
 * @param [in]: num: the original number that to aligned
 * @param [in]: align: the align factor
 * @return the number after aligned
 */
#define ALIGN_UP(num, align) (((num) + (align) - 1) & ~((align) - 1))

/**
 * @brief calculate number align with 2
 * @param [in]: num: the original number that to aligned
 * @return the number after aligned
 */
#define ALIGN_UP2(num) ALIGN_UP(num, 2)

/**
 * @brief calculate number align with 16
 * @param [in]: num: the original number that to aligned
 * @return the number after aligned
 */
#define ALIGN_UP16(num) ALIGN_UP(num, 16)

/**
 * @brief calculate number align with 64
 * @param [in]: num: the original number that to aligned
 * @return the number after aligned
 */
#define ALIGN_UP64(num) ALIGN_UP(num, 64)

/**
 * @brief calculate number align with 128
 * @param [in]: num: the original number that to aligned
 * @return the number after aligned
 */
#define ALIGN_UP128(num) ALIGN_UP(num, 128)

/**
 * @brief calculate elements num of array
 * @param [in]: array: the array variable
 * @return elements num of array
 */
#define SIZEOF_ARRAY(array)  (sizeof(array)/sizeof(array[0]))

/**
 * @brief Write acl error level log to host log
 * @param [in]: fmt: the input format string
 * @return none
 */
#define ACLLITE_LOG_ERROR(fmt, ...) \
    do { fprintf(stdout, "[ERROR]  " fmt "\n", ##__VA_ARGS__);}while (0)

/**
 * @brief Write acl info level log to host log
 * @param [in]: fmt: the input format string
 * @return none
 */
#define ACLLITE_LOG_INFO(fmt, ...) \
    do { fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__);}while (0)

/**
 * @brief Write acl warining level log to host log
 * @param [in]: fmt: the input format string
 * @return none
 */
#define ACLLITE_LOG_WARNING(fmt, ...) \
    do { fprintf(stdout, "[WARNING]  " fmt "\n", ##__VA_ARGS__);}while (0)

/**
 * @brief Write acl debug level log to host log
 * @param [in]: fmt: the input format string
 * @return none
 */
#define ACLLITE_LOG_DEBUG(fmt, ...) \
    do { fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__);}while (0)

/**
 * @brief define variable record time &&
          set start time
 * @param [X]: function name
 * @return X_START X_END
 */
#define TIME_START(X) \
    auto X##_START = std::chrono::steady_clock::now(), \
    X##_END = X##_START

/**
 * @brief set end time
 * @param [X]: function name
 * @return none
 */
#define TIME_END(X) \
    X##_END = std::chrono::steady_clock::now() \

/**
 * @brief calculate time by nanosecond
 * @param [X]: function name
 * @return none
 */
#define TIME_NSEC(X) \
    std::chrono::duration_cast<std::chrono::nanoseconds> \
    (X##_END - X##_START).count()

/**
 * @brief show time by nanosecond
 * @param [X]: function name
 * @return none
 */
#define TIME_NSEC_SHOW(X) \
    cout << "Func " << #X << " cost : " << TIME_NSEC(X) \
    << " ns " << endl

/**
 * @brief calculate time and show by microsecond
 * @param [X]: variable name
 * @return none
 */
#define TIME_USEC(X) \
    std::chrono::duration_cast<std::chrono::microseconds> \
    (X##_END - X##_START).count()

/**
 * @brief show time by microsecond
 * @param [X]: function name
 * @return none
 */
#define TIME_USEC_SHOW(X) \
    cout << "Func " << #X << " cost : " << TIME_USEC(X) \
    << " us " << endl

/**
 * @brief calculate time and show by millisecond
 * @param [X]: variable name
 * @return none
 */
#define TIME_MSEC(X) \
    std::chrono::duration_cast<std::chrono::milliseconds> \
    (X##_END - X##_START).count()

/**
 * @brief show time by millisecond
 * @param [X]: function name
 * @return none
 */
#define TIME_MSEC_SHOW(X) \
    cout << "Func " << #X << " cost : " << TIME_MSEC(X) \
    << " ms " << endl

/**
 * @brief calculate time and show by second
 * @param [X]: variable name
 * @return none
 */
#define TIME_SEC(X) \
    std::chrono::duration_cast<std::chrono::seconds> \
    (X##_END - X##_START).count()

/**
 * @brief show time by second
 * @param [X]: function name
 * @return none
 */
#define TIME_SEC_SHOW(X) \
    cout << "Func " << #X << " cost : " << TIME_SEC(X) \
    << " s " << endl

/**
 * @brief calculate time and show by minute
 * @param [X]: variable name
 * @return none
 */
#define TIME_MINUTE(X) \
    std::chrono::duration_cast<std::chrono::minutes> \
    (X##_END - X##_START).count()

/**
 * @brief show time by minute
 * @param [X]: function name
 * @return none
 */
#define TIME_MINUTE_SHOW(X) \
    cout << "Func " << #X << " cost : " << TIME_MINUTE(X) \
    << " min " << endl

/**
 * @brief calculate time and show by hour
 * @param [X]: variable name
 * @return none
 */
#define TIME_HOUR(X) \
    std::chrono::duration_cast<std::chrono::hours> \
    (X##_END - X##_START).count()

/**
 * @brief show time by hour
 * @param [X]: function name
 * @return none
 */
#define TIME_HOUR_SHOW(X) \
    cout << "Func " << #X << " cost : " << TIME_HOUR(X) \
    << " h " << endl

/**
 * @brief Recognize the string is a accessable directory or not
 * @param [in]: path: the input string
 * @return bool  true: is directory; false: not directory
 */
bool IsDirectory(const std::string &path);


/**
 * @brief Read jpeg image file. Only support baseline, not support progressive
 * @param [out]: image: image data read from file.
 * @param [in]: fileName: The data bytes size
 * @return AclLiteError ACLLITE_OK: read success
 *                    others: read failed
 */
// int ReadJpeg(ImageData& image, const std::string& fileName);

// int ReadPng(ImageData& image, const std::string& fileName);

/**
 * @brief Get all files from file list string
 * @param [in]: pathList: files list string, seperate by ',',
 *                   the element could be file path or directory
 * @param [in]: fileVec: The data bytes size
 * @return AclLiteError ACLLITE_OK: read success
 *                    others: read failed
 */
void GetAllFiles(const std::string &pathList,
                 std::vector<std::string> &fileVec);

/**
 * @brief Save data to binary file
 * @param [in]: filename: binary file name with path
 * @param [in]: data: binary data
 * @param [in]: size: bytes size of data
 * @return AclLiteError ACLLITE_OK: read success
 *                    others: read failed
 */
void SaveBinFile(const std::string& filename, const void* data, uint32_t size);

/**
 * @brief Read binary file to buffer
 * @param [in]: filename: binary file name with path
 * @param [in]: data: buffer
 * @param [in]: size: buffer size
 * @return AclLiteError ACLLITE_OK: read success
 *                    others: read failed
 */
int ReadBinFile(const std::string& filename,
                         void*& data, uint32_t& size);



/**
 * @brief Match ip address string as <1-255>.<0-255>.<0-255>.<0-255>:<port>
 * @param [in]: addrStr: Ip address string
 * @return bool true: The input string match success
 *              false: is not match
 */
bool IsIpAddrWithPort(const std::string& addrStr);

/**
 * @brief Split ip address string <1-255>.<0-255>.<0-255>.<0-255>:<port> to
 *        ip and port
 * @param [out]: ip: Ip address <1-255>.<0-255>.<0-255>.<0-255>
 * @param [out]: port: port string
 * @param [in]: addr: Ip address string
 * @return None
 */
void ParseIpAddr(std::string& ip, std::string& port, const std::string& addr);

/**
 * @brief Judge input string is mp4 file path
 * @param [in]: path: file path
 * @return bool true: input string is mp4 file path
 *              false: is not mp4 file path
 */
bool IsVideoFile(const std::string& path);

/**
 * @brief Judge input string is rtsp addr link rtsp://
 * @param [in]: str: input string
 * @return bool true: input string is rtsp address
 *              false: is not rtsp address
 */
bool IsRtspAddr(const std::string &str);

/**
 * @brief Judge input string is digit string
 * @param [in]: str: input string
 * @return bool true: input string is digit string
 *              false: is not rtsp address
 */
bool IsDigitStr(const std::string& str);

/**
 * @brief Test file path is exist or not
 * @param [in]: path: file path
 * @return bool true: file path is exist
 *              false: is not exist
 */
bool IsPathExist(const std::string &path);

/**
 * @brief read file and save information to config
 * @param [out]: config: map, save option information
 * @param [in]: configFile: string, file
 * @return bool true: read config success
 *              false: read config fail
 */
bool ReadConfig(std::map<std::string, std::string>& config,
                const char* configFile);

/**
 * @brief print option information
 * @param [in]: m: map, save option information
 * @return None
 */
void PrintConfig(const std::map<std::string, std::string> & m);