#include "PostprocessStructs.hlsli"
#include "common.hlsli"

Texture2D orthographicWorldTexture : register(t1);

PostProcessPixelOutput main(PostProcessVertexToPixel input)
{
    float4 worldPositionOfPixel = orthographicWorldTexture.Sample(DefaultSampler, input.myUV);
    if (worldPositionOfPixel.x == 0.0f && worldPositionOfPixel.y == 0.0f && worldPositionOfPixel.z == 0.0f)
    {
        discard;
    }
    worldPositionOfPixel /= 100.0f;
    
    float2 distanceVec = float2(worldPositionOfPixel.x - customShaderParameters.x, worldPositionOfPixel.z - customShaderParameters.y);
    if (sqrt((distanceVec.x * distanceVec.x) + (distanceVec.y * distanceVec.y)) > customShaderParameters.z)
    {
        discard;
    }
    
    PostProcessPixelOutput result;
    result.myColor = float4(0.685f, 0.6f, 0.4f, 1.0f); // Placeholder parchment color until we have a texture to sample from
    result.myColor.rgb *= 0.25f + 0.75f * (1.0f - smoothstep(4.0f, -3.0f, worldPositionOfPixel.y));
    return result;
}