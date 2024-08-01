#include "TextureLoader.h"

#include "Logging.h"

#include <Render/Textures.h>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

void LoadTextureThread()
{

}

tpr::Texture_t LoadTextureFromBinary(const void* pData, size_t size)
{
    int x, y, comp;
    stbi_uc* loadedTex = stbi_load_from_memory((stbi_uc*)pData, (int)size, &x, &y, &comp, 4);

    if (loadedTex == nullptr)
    {
        LOGERROR("LoadTextureFromBinary : Failed to load texture");
        return tpr::Texture_t::INVALID;
    }

    tpr::TextureCreateDesc texDesc = {};
    texDesc.Width = x;
    texDesc.Height = y;
    texDesc.Format = tpr::RenderFormat::R8G8B8A8_UNORM;
    texDesc.Flags = tpr::RenderResourceFlags::SRV;
    
    tpr::MipData mip0(loadedTex, texDesc.Format, texDesc.Width, texDesc.Height);

    texDesc.Data = &mip0;

    tpr::Texture_t tex = tpr::CreateTexture(texDesc);

    stbi_image_free(loadedTex);

    return tex;
}
