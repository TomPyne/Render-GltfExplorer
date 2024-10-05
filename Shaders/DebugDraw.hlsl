#include "Shaders/ViewData.h"

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

#ifdef _VS

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

void main(in VS_INPUT input, out PS_INPUT output)
{
    output.pos = mul(float4(input.pos, 1.0f), View.ViewProjectionMatrix);
    output.color = input.color;
}

#endif // #ifdef _VS

#ifdef _PS

float4 main(in PS_INPUT input) : SV_TARGET
{
    return input.color;
}

#endif // #ifdef _PS