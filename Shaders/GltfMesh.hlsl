cbuffer viewData : register(b0) 
{
    row_major float4x4 ViewProjectionMatrix; 
    float3 CameraPosition;
    float pad0;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 texcoord[2] : TEXCOORD;
};

#ifdef _VS

cbuffer modelData : register(b1) 
{
    row_major float4x4 ModelMatrix; 
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord0 : TEXCOORD;
    float2 texcoord1 : TEXCOORD1;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.worldPos = mul(float4(input.pos, 1.f), ModelMatrix).xyz;
    output.pos = mul(float4(output.worldPos, 1.0f), ViewProjectionMatrix);
    output.normal = mul(float4(input.normal, 0.0f), ModelMatrix).xyz;
    output.tangent = mul(float4(input.tangent.xyz, 0.0f), ModelMatrix).xyz;
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    output.texcoord[0] = input.texcoord0;
    output.texcoord[1] = input.texcoord1;
    return output;
}

#endif

#ifdef _PS

cbuffer MaterialData : register(b2)
{
    float4 BaseColorFactor;

    float3 EmissiveFactor;
    float MaskAlphaCutoff;

    uint BaseColorTextureIndex;
    uint NormalTextureIndex;    
    uint MetallicRoughnessTextureIndex;
    uint EmissiveTextureIndex;

    uint BaseColorUvIndex;
    uint NormalUvIndex;
    uint MetallicRoughnessUvIndex;
    uint EmissiveUvIndex;

    float MetallicFactor;
    float RoughnessFactor;

    float2 pad1;
};

#if _BINDLESS

Texture2D<float4> t_tex2d[512] : register(t0, space0);

#define MTL_TEX(BoundTex, BindlessIndex) t_tex2d[BindlessIndex]

#else

Texture2D<float4> BaseColorTexture : register(t0);
Texture2D<float4> NormalTexture : register(t1);
Texture2D<float4> MetallicRoughnessTexture : register(t2);
Texture2D<float4> EmissiveTexture : register(t3);

#define MTL_TEX(BoundTex, BindlessIndex) BoundTex

#endif // _BINDLESS

SamplerState TrilinearSampler : register(s0);

static const float M_PI = 3.14159265359;

float3 F_Schlick(float3 f0, float3 f90, float VdotH)
{
    return f0 + (f90 - f0) * pow(saturate(1.0f - VdotH), 5.0f);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (M_PI * f * f);
}

//https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
float3 BRDF_lambertian(float3 f0, float3 f90, float3  diffuseColor, float specularWeight, float VdotH)
{
    // see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
    return (1.0f - specularWeight * F_Schlick(f0, f90, VdotH)) * (diffuseColor / M_PI);
}

//  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
float3 BRDF_specularGGX(float3 f0, float3 f90, float alphaRoughness, float specularWeight, float VdotH, float NdotL, float NdotV, float NdotH)
{
    float3 F = F_Schlick(f0, f90, VdotH);
    float Vis = V_GGX(NdotL, NdotV, alphaRoughness);
    float D = D_GGX(NdotH, alphaRoughness);

    return specularWeight * F * Vis * D;
}

float3 Tonemap(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(PS_INPUT input, bool frontFace : SV_IsFrontFace) : SV_Target
{
    float4 baseColor = BaseColorFactor;
    if(BaseColorTextureIndex)
    {
        baseColor *= MTL_TEX(BaseColorTexture, BaseColorTextureIndex).Sample(TrilinearSampler, input.texcoord[BaseColorUvIndex]);
    }

    float3 vertexNormal = normalize(frontFace ? input.normal : -input.normal);

    float3 normal = vertexNormal;
    if(NormalTextureIndex)
    {
        float3 texNormal = MTL_TEX(NormalTexture, NormalTextureIndex).Sample(TrilinearSampler, input.texcoord[NormalUvIndex]).rgb;

        texNormal = (2.0f * texNormal) - float(1.0f).rrr;

        float3x3 tangentMatrix = float3x3(input.tangent, input.bitangent, vertexNormal);

        normal = normalize(mul(texNormal, tangentMatrix));
    }

    float metallic = MetallicFactor;
    float roughness = RoughnessFactor;
    if(MetallicRoughnessTextureIndex)
    {
        float2 texMetallicRoughness = MTL_TEX(MetallicRoughnessTexture, MetallicRoughnessTextureIndex).Sample(TrilinearSampler, input.texcoord[MetallicRoughnessUvIndex]).gb;

        metallic *= texMetallicRoughness.y;
        roughness *= texMetallicRoughness.x;    
    }
    
    float3 emissive = EmissiveFactor;
    if(EmissiveTextureIndex)
    {
        emissive *= MTL_TEX(EmissiveTexture, EmissiveTextureIndex).Sample(TrilinearSampler, input.texcoord[EmissiveUvIndex]).rgb;
    }

    const float3 LightDir = normalize(float3(0.5f, -1.0f, 0.5f));
    const float3 LightAmbient = float3(0.3f, 0.3f, 0.3f);
    const float3 LightRadiance = float3(3.0f, 3.0f, 4.0f);
    
    const float3 n = normalize(normal);
    const float3 v = normalize(input.worldPos - CameraPosition);
    const float3 l = LightDir;
    const float3 h = normalize(l + v);

    const float vdh = saturate(dot(v, h));
    const float ndl = saturate(dot(n, l));
    const float ndv = saturate(dot(n, v));
    const float ndh = saturate(dot(n, h));

    static const float3 black = 0.0f;
    static const float3 f0Dielectric = 0.04f;

    const float3 f0 = lerp(f0Dielectric, baseColor.rgb, metallic);
    const float3 f90 = float3(1.0f, 1.0f, 1.0f);

    const float3 adjustedRadiance = LightRadiance * max(ndl, 0);

    float3 spec = 0;
    float3 diff = 0;

    diff += LightAmbient * baseColor.rgb;

    if(ndl > 0 || ndv > 0)
    {   
        float alphaRoughness = saturate(roughness);
        alphaRoughness = alphaRoughness * alphaRoughness;

        diff += LightRadiance * ndl * BRDF_lambertian(f0, f90, baseColor.rgb, 1.0f, vdh );
        spec += LightRadiance * ndl * BRDF_specularGGX(f0, f90, alphaRoughness, 1.0f, vdh, ndl, ndv, ndh);      
    }

    return float4(Tonemap(diff + spec + emissive), baseColor.a); 

}

#endif