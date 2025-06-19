#include "PostprocessStructs.hlsli"
#include "common.hlsli"

Texture2D orthographicWorldTexture : register(t1);
Texture2D mapColorTexture : register(t2);

PostProcessPixelOutput main(PostProcessVertexToPixel input)
{
    PostProcessPixelOutput result;
    
    float4 worldPositionOfPixel = orthographicWorldTexture.Sample(DefaultSampler, input.myUV);
    if (worldPositionOfPixel.x == 0.0f && worldPositionOfPixel.y == 0.0f && worldPositionOfPixel.z == 0.0f)
    {
        discard;
    }
    worldPositionOfPixel /= 100.0f;
    
    float2 distanceVector = float2(worldPositionOfPixel.x - customShaderParameters.x, worldPositionOfPixel.z - customShaderParameters.y);
    if ((distanceVector.x * distanceVector.x) + (distanceVector.y * distanceVector.y) > customShaderParameters.z)
    {
        discard;
    }
    
    // Future improvement: Tiling snaps awkwardly when min/max camera state swaps, would be better to calculate albedo tiling based on static world
    float2 tiledColorUVs = input.myUV * customShaderParameters.w;
    result.myColor = mapColorTexture.Sample(DefaultSampler, tiledColorUVs);
    result.myColor.rgb *= 0.25f + 0.75f * (1.0f - smoothstep(8.0f, -8.0f, worldPositionOfPixel.y));
    
    return result;
}