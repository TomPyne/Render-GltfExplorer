#pragma once

#include <cstdint>
#include <Render/RenderTypes.h>

enum class ShadowMapFormat : uint8_t
{
	FLOAT32,
	UNORM16,
	UNORM24S8,
};

struct ShadowMapSettings
{
	uint32_t Size = 4096;
	ShadowMapFormat Format = ShadowMapFormat::FLOAT32;
};

struct ShadowMap
{
	tpr::TexturePtr ShadowTexture = {};
	tpr::ShaderResourceViewPtr ShadowSrv = {};
	tpr::DepthStencilView_t ShadowDsv = {};


};