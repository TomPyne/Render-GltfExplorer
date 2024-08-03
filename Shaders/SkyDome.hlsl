#include "Shaders/ViewData.h"

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 uvw : UVW;    
};

#ifdef _VS

struct VS_INPUT
{
    float3 pos : POSITION;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float3 worldPos = input.pos + View.CameraPosition;
    output.pos = mul(float4(worldPos, 1.0f), View.ViewProjectionMatrix);
    
    output.uvw = normalize(input.pos);
    output.uv = float2(atan2(output.uvw.z, output.uvw.x), asin(-output.uvw.y)) * float2(0.1591, 0.3183) + float2(0.5, 0.5);

    return output;
}

#endif // _VS

#ifdef _PS

float3 Tonemap(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

#if _BINDLESS

cbuffer MaterialData : register(b2)
{
    uint SrvIndex;
    float3 _pad;
}

Texture2D<float4> t_tex2d[512] : register(t0, space0);
#else // !_BINDLESS
Texture2D<float4> Texture : register(t0);
#endif // _BINDLESS

float4 main(PS_INPUT input) : SV_Target
{
    float3 uvw = normalize(input.uvw);

    input.uv = float2(atan2(input.uvw.z, input.uvw.x), asin(-input.uvw.y)) * float2(0.1591, 0.3183) + float2(0.5, 0.5);

#if _BINDLESS
    float3 color = t_tex2d[SrvIndex].Sample(TrilinearSampler, input.uv).rgb;
#else
    float3 color = Texture.Sample(TrilinearSampler, input.uv).rgb;
#endif

    return float4(Tonemap(color), 1.0f);
}

#endif // _PS