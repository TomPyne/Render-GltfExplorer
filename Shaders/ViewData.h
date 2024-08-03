cbuffer viewData : register(b0) 
{
    row_major float4x4 ViewProjectionMatrix; 
    float3 CameraPosition;
    float pad0;
};

SamplerState TrilinearSampler : register(s0);