#include "TextureLoader.h"

#include "Logging.h"

#include <Render/Textures.h>
#include <thread>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

struct FileHandle
{
    explicit FileHandle(const char* pFileName)
    {
        if (!(fopen_s(&pFile, pFileName, "rb") == 0))
        {
            pFile = nullptr;
        }
    }

    size_t GetSize() const
    {
        if (!pFile)
        {
            return 0;
        }

        if (fseek(pFile, 0, SEEK_END) < 0)
        {
            return 0;
        }

        const size_t size = ftell(pFile);

        if (fseek(pFile, 0, SEEK_SET) < 0)
        {
            return 0;
        }

        return size;
    }

    void ReadIntoMem(void* pMem, size_t memSize)
    {
        fread_s(pMem, memSize, sizeof(unsigned char), memSize, pFile);
    }

    ~FileHandle()
    {
        fclose(pFile);
    }

    FILE* pFile = nullptr;

    operator bool() const { return pFile != nullptr; }
};

std::unique_ptr<stbi_uc[]> LoadBinaryFile(const char* const pFileName, size_t& size)
{
    FileHandle file = FileHandle(pFileName);
    if (!file)
    {
        return {};
    }

    size = file.GetSize();

    if (size <= 0)
    {
        return {};
    }       

    std::unique_ptr<stbi_uc[]> pRet = std::make_unique<stbi_uc[]>(size);

    file.ReadIntoMem(pRet.get(), size);

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
