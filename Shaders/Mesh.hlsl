struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

#ifdef _VS

cbuffer viewBuf : register(b0)
{
    float4x4 ViewProjectionMatrix;
};

cbuffer meshBuf : register(b1)
{
    row_major float4x4 MeshTransformMatrix;
}

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = mul(MeshTransformMatrix, float4(input.pos.xyz, 1.f));
    
    output.pos = mul(ViewProjectionMatrix, worldPos);
    output.color = input.color;

    return output;
};

#endif

#ifdef _PS

float4 main(PS_INPUT input) : SV_Target0
{
    return float4(input.color, 1.0f);
};

#endif