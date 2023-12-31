#pragma once

#include <unistd.h>
#include <string>
#include <memory>
#include "utils.h"

void SetPixel(ImageData &image, int x, int y, const YUVColor &color);
void RenderText(ImageData &image, int x, int y, const std::string &text, const YUVColor *color);

