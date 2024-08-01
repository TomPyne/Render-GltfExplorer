#include "TextureLoader.h"

#include "Logging.h"

#include <Render/Textures.h>
#include <thread>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

std::unique_ptr<stbi_uc[]> LoadBinaryFile(const char* const pFileName, size_t& size)
{
    FILE* pFile = nullptr;
    if (!(fopen_s(&pFile, pFileName, "rb") == 0))
        return nullptr;

    if (fseek(pFile, 0, SEEK_END) < 0)
    {
        fclose(pFile);
        return nullptr;
    }

    size = ftell(pFile);

    if (size == 0)
    {
        fclose(pFile);
        return nullptr;
    }

    rewind(pFile);

    std::unique_ptr<stbi_uc[]> pRet = std::make_unique<stbi_uc[]>(size);

    memcpy(pRet.get(), pFile, size);

    fclose(pFile);

    return pRet;
}

tpr::TexturePtr LoadTextureFromFile(const char* const pFileName)
{
    size_t size;
    std::unique_ptr<uint8_t[]> pBinary = LoadBinaryFile(pFileName, size);

    if (pBinary)
    {
        return LoadTextureFromBinary(pBinary.get(), size);
    }

    return {};
}

tpr::TexturePtr LoadHdrTextureFromFile(const char* const pFileName)
{
    size_t size;
    std::unique_ptr<uint8_t[]> pBinary = LoadBinaryFile(pFileName, size);

    if (pBinary)
    {
        return LoadHdrTextureFromBinary(pBinary.get(), size);
    }

    return {};
}

tpr::TexturePtr LoadTextureFromBinary(const void* pData, size_t size)
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

    tpr::TexturePtr tex = tpr::CreateTexture(texDesc);

    stbi_image_free(loadedTex);

    return tex;
}

tpr::TexturePtr LoadHdrTextureFromBinary(const void* const pData, size_t size)
{
    int x, y, comp;
    float* loadedTex = stbi_loadf_from_memory((stbi_uc*)pData, (int)size, &x, &y, &comp, 4);

    if (loadedTex == nullptr)
    {
        LOGERROR("LoadTextureFromBinary : Failed to load texture");
        return tpr::Texture_t::INVALID;
    }

    tpr::TextureCreateDesc texDesc = {};
    texDesc.Width = x;
    texDesc.Height = y;
    texDesc.Format = tpr::RenderFormat::R32G32B32A32_FLOAT;
    texDesc.Flags = tpr::RenderResourceFlags::SRV;

    tpr::MipData mip0(loadedTex, texDesc.Format, texDesc.Width, texDesc.Height);

    texDesc.Data = &mip0;

    tpr::TexturePtr tex = tpr::CreateTexture(texDesc);

    stbi_image_free(loadedTex);

    return tex;
}
