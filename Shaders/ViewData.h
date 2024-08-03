struct ViewData
{
    row_major float4x4 ViewProjectionMatrix; 
    float3 CameraPosition;
    float pad0;
    float3 SunDirection;
    float pad1;
    float3 SunRadiance;
    float pad2;
};

cbuffer ViewCBuf : register(b0)
{
    ViewData View;
}

SamplerState TrilinearSampler : register(s0);