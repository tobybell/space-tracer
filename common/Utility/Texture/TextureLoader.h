#pragma once

#ifndef __TEXTURE_LOADER__
#define __TEXTURE_LOADER__

#include "common/common.h"

class Texture2D;

namespace TextureLoader
{
unsigned char* LoadRawData(const std::string& filename, int& width, int& height);
std::shared_ptr<Texture2D> LoadTexture(const std::string& filename);

}
#endif
