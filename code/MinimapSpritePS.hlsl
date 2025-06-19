#include "common.hlsli"

SamplerState SampleType;
Texture2D culledWorldTexture : register(t1);

float4 main(ModelVertexToPixel input) : SV_TARGET
{
    float2 uv = input.texCoord0;
    uv.x = customShaderParameters.z - customShaderParameters.x + (customShaderParameters.x * 2.0f * uv.x);
    uv.y = customShaderParameters.w - customShaderParameters.y + (customShaderParameters.y * 2.0f * uv.y);
    
    float2 pixelOffset = float2(ddx(uv.x), ddy(uv.y));
    float4 p00 = culledWorldTexture.Sample(SampleType, uv + pixelOffset * float2(-0.5f, -0.5f));
    float4 p01 = culledWorldTexture.Sample(SampleType, uv + pixelOffset * float2(-0.5f, 0.5f));
    float4 p10 = culledWorldTexture.Sample(SampleType, uv + pixelOffset * float2(0.5f, -0.5f));
    float4 p11 = culledWorldTexture.Sample(SampleType, uv + pixelOffset * float2(0.5f, 0.5f));
 
    float4 blendedAlbedo = 0.25f * (p00 + p01 + p10 + p11);
		
    if (blendedAlbedo.a <= 0.0f)
    {
        discard;
    }

    return blendedAlbedo;
}