
#pragma once

#include <unistd.h>
#include <string>
#include <memory>

typedef int PixelFormat;

class YUVColor
{
public:
    YUVColor(uint8_t y, uint8_t u, uint8_t v) : y(y), u(u), v(v) {}
    uint8_t y;
    uint8_t u;
    uint8_t v;
};

typedef struct _ImageData
{
    PixelFormat format;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t alignWidth = 0;
    uint32_t alignHeight = 0;
    uint32_t size = 0;
    std::shared_ptr<uint8_t> data = nullptr;
} ImageData;

struct FrameData
{
    bool isFinished = false;
    uint32_t frameId = 0;
    uint32_t size = 0;
    void *data = nullptr;
};

struct Resolution
{
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Rect
{
    uint32_t ltX = 0;
    uint32_t ltY = 0;
    uint32_t rbX = 0;
    uint32_t rbY = 0;
};

struct BBox
{
    Rect rect;
    uint32_t score = 0;
    std::string text;
};