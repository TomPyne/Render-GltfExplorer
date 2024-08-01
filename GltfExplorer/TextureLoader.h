#pragma once

#include <Render/RenderTypes.h>

tpr::TexturePtr LoadTextureFromFile(const char* const pFileName);
tpr::TexturePtr LoadHdrTextureFromFile(const char* const pFileName);

tpr::TexturePtr LoadTextureFromBinary(const void* const pData, size_t size);
tpr::TexturePtr LoadHdrTextureFromBinary(const void* const pData, size_t size);

