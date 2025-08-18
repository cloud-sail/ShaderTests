#include "Common/ShaderConstants.hlsli"
#include "Common/Resources.hlsli"
#include "Common/Math.hlsli"
#include "Common/Utils.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/StaticSampler.hlsli"
#include "Common/TriplanarUtils.hlsli"
#include "Common/ToneMapping.hlsli"


#define THREADS_PER_GROUP_SIZE (8)
static const float INFINITY_DIST = 1e35f;


struct SdfRayMarchingResources
{
    uint engineConstantsIndex;
    uint cameraConstantsIndex;
    uint modelConstantsIndex;
    uint lightConstantsIndex;
    uint perFrameConstantsIndex;

    uint inputSdfShapesIndex; // StructuredBuffer<SdfShape>
    uint outputTextureIndex;  // RWTexture2D<float4>
    uint outputDepthIndex;     
    uint rayMarchingConstantsIndex;
};


struct SdfRayMarchingConstants
{
	int maxSteps;
	float minHitDistance;
	float maxTraceDistance;
	float toleranceK;

	int numOfShapes;
    int screenWidth;
    int screenHeight;
    float padding;

    float triplanarUVScale;
	float triplanarBlendSharpness;
	float padding1;
	float padding2;
};


struct SdfShape
{
	int m_type;     // 0 sphere
	int m_padding0;
	int m_padding1;
	uint m_triAlbedoTexID;

    uint m_triMRTexID;
	uint m_triNormalTexID;
	uint m_triOcclusionTexID;
	uint m_triEmissiveTexID;

    float4 color;
    float4 data0; // xyz: center w: radius
};

ConstantBuffer<SdfRayMarchingResources> renderResources : register(b0);

/*
1. Masking: MaskA only process materialA and materialB, MaskB only processes materialB and material C
2. Weight Thresholding: if weight < ? ignore the material
3. Top N

*/

//------------------------------------------------------------------------------------
float sdSphere(float3 p, float3 c, float r)
{
    return length(p - c) - r;
}


float sdfValueFromShape(float3 p, SdfShape s)
{
    // Sphere
    if (s.m_type == 0)
    {
        return sdSphere(p, s.data0.xyz, s.data0.w);
    }


    return INFINITY_DIST;
}


// Union: min(a,b)
// Intersection: max(a,b)
// Subtraction: max(a, -b)
// https://iquilezles.org/articles/smin/
float sminQuad(float a, float b, float k)
{
    k *= 4.0;
    float h = max( k-abs(a-b), 0.0 )/k;
    return min(a,b) - h*h*k*(1.0/4.0);
}

float sminCubic(float a, float b, float k)
{
    k *= 6.0;
    float h = max( k-abs(a-b), 0.0 )/k;
    return min(a,b) - h*h*h*k*(1.0/6.0);
}

//-------------------------------------------------------------------------------------------
// TODO: not just union of sdfs, but also subtraction and intersection
// Sample SDF value from input position
float SdfMap(float3 p)
{
    StructuredBuffer<SdfShape> sdfShapes = ResourceDescriptorHeap[renderResources.inputSdfShapesIndex];
    ConstantBuffer<SdfRayMarchingConstants>   sdfConstants = ResourceDescriptorHeap[renderResources.rayMarchingConstantsIndex];

    const int numOfShapes = sdfConstants.numOfShapes;
    const float toleranceK = sdfConstants.toleranceK;

    float res = INFINITY_DIST;
    for (int i = 0; i < numOfShapes; ++i)
    {
        res = sminCubic(res, sdfValueFromShape(p, sdfShapes[i]), toleranceK);
    }
    return res;
}


// https://iquilezles.org/articles/normalsSDF/
// Tetrahedron technique
float3 SdfNormalTetra(float3 p)
{
    const float h = 0.0001f;
    const float2 k = float2(1.f,-1.f);
    return normalize( k.xyy*SdfMap( p + k.xyy*h ) + 
                      k.yyx*SdfMap( p + k.yyx*h ) + 
                      k.yxy*SdfMap( p + k.yxy*h ) + 
                      k.xxx*SdfMap( p + k.xxx*h ) );
}

float3 GetWeightedColor(float3 p, float3 backgroundColor, float3 worldNormal)
{
    // TODO: Input Normal To Sample from Triplanar Mapping

    StructuredBuffer<SdfShape> sdfShapes = ResourceDescriptorHeap[renderResources.inputSdfShapesIndex];
    ConstantBuffer<SdfRayMarchingConstants>   sdfConstants = ResourceDescriptorHeap[renderResources.rayMarchingConstantsIndex];

    const int numOfShapes = sdfConstants.numOfShapes;
    const float toleranceK = sdfConstants.toleranceK;

    float threshold = toleranceK * 3.f;

    float3 colorSum = float3(0,0,0);
    float weightSum = 0.0f;

    for (int i = 0; i < numOfShapes; ++i)
    {
        float d = sdfValueFromShape(p, sdfShapes[i]);

        float3 c = sdfShapes[i].color.rgb;

        if (sdfShapes[i].m_triAlbedoTexID != INVALID_INDEX)
        {
            Texture2D<float4> diffuseTexture = ResourceDescriptorHeap[sdfShapes[i].m_triAlbedoTexID];

            c = SampleTriplanar(p, worldNormal, sdfConstants.triplanarUVScale, sdfConstants.triplanarBlendSharpness, diffuseTexture, s_linearWrap).rgb;
        }

        if (d < threshold)
        {
            float w = max(0, threshold - d); // linear weight, maybe exp(-d/k)
            colorSum += c * w;
            weightSum += w;
        }
    }

    if (weightSum > 0.0f)
        return colorSum / weightSum;
    else
        return backgroundColor; // or background color
}

SurfaceData GetWeightedSurfaceData(float3 p, float3 worldNormal)
{
    StructuredBuffer<SdfShape> sdfShapes = ResourceDescriptorHeap[renderResources.inputSdfShapesIndex];
    ConstantBuffer<SdfRayMarchingConstants>   sdfConstants = ResourceDescriptorHeap[renderResources.rayMarchingConstantsIndex];

    const int numOfShapes = sdfConstants.numOfShapes;
    const float toleranceK = sdfConstants.toleranceK;

    float threshold = toleranceK * 3.f;

    float3 albedoSum = 0.f;
    float3 normalSum = 0.f;
    float metallicSum = 0.f;
    float roughnessSum = 0.f;
    float3 emissionSum = 0.f;
    float AOSum = 0.f;

    float weightSum = 0.0f;

    SurfaceData surf = MakeDefaultSurfaceData();

    for (int i = 0; i < numOfShapes; ++i)
    {
        float d = sdfValueFromShape(p, sdfShapes[i]);

        if (d > threshold)
        {
            continue;
        }

        Texture2D<float4> albedoTexture = ResourceDescriptorHeap[sdfShapes[i].m_triAlbedoTexID];
        Texture2D<float4> metalicRoughnessTexture = ResourceDescriptorHeap[sdfShapes[i].m_triMRTexID];
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[sdfShapes[i].m_triNormalTexID];
        Texture2D<float4> occlusionTexture = ResourceDescriptorHeap[sdfShapes[i].m_triOcclusionTexID];
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[sdfShapes[i].m_triEmissiveTexID];
	    SamplerState samp = s_linearWrap;

        float4 albedoTexel = SampleTriplanar(p, worldNormal, sdfConstants.triplanarUVScale, sdfConstants.triplanarBlendSharpness,
            albedoTexture, samp);
        if (albedoTexel.a < 0.01f)
        {
            continue;
        }

        float2 metalicRoughness = SampleTriplanar(p, worldNormal, sdfConstants.triplanarUVScale, sdfConstants.triplanarBlendSharpness,
            metalicRoughnessTexture, samp).bg;
        float occlusion = SampleTriplanar(p, worldNormal, sdfConstants.triplanarUVScale, sdfConstants.triplanarBlendSharpness,
            occlusionTexture, samp).r;
        float3 emissive = SampleTriplanar(p, worldNormal, sdfConstants.triplanarUVScale, sdfConstants.triplanarBlendSharpness,
            emissiveTexture, samp).rgb;

        float3 pixelNormalWorldSpace = SampleTriplanarNormal(p, worldNormal, sdfConstants.triplanarUVScale, sdfConstants.triplanarBlendSharpness,
            normalTexture, samp);


        float w = max(0, threshold - d); // linear weight, maybe exp(-d/k)
        weightSum += w;

        albedoSum += w * albedoTexel.rgb;
        normalSum += w * pixelNormalWorldSpace.xyz;
        metallicSum += w * metalicRoughness.x;
        roughnessSum += w * metalicRoughness.y;
        emissionSum += w * emissive;
        AOSum += w * occlusion;
    }

    if (weightSum > 0.0f)
    {
        surf.Albedo = albedoSum / weightSum;
        surf.Normal = normalize(normalSum);
        surf.Metallic = metallicSum / weightSum;
        surf.Roughness = roughnessSum / weightSum;
        surf.Emission = emissionSum / weightSum;
        surf.AO = AOSum / weightSum;
    }

    return surf;
}



// ray march
// in: rayStartPos rayFwdNormal
// out: color.rgb and distance (float4)
float4 RayMarch(float3 rayStartPos, float3 rayFwdNormal)
{
    ConstantBuffer<LightConstants>      lightConstants = ResourceDescriptorHeap[renderResources.lightConstantsIndex];
    ConstantBuffer<SdfRayMarchingConstants> sdfConstants = ResourceDescriptorHeap[renderResources.rayMarchingConstantsIndex];
    ConstantBuffer<EngineConstants>     engineConstants = ResourceDescriptorHeap[renderResources.engineConstantsIndex];
    ConstantBuffer<CameraConstants>     cameraConstants = ResourceDescriptorHeap[renderResources.cameraConstantsIndex];

    StructuredBuffer<SdfShape> sdfShapes = ResourceDescriptorHeap[renderResources.inputSdfShapesIndex];


    const int maxSteps = sdfConstants.maxSteps;
    const float minHitDistance = sdfConstants.minHitDistance;
    const float maxTraceDistance = sdfConstants.maxTraceDistance;

    const float3 missingColor = float3(0.2f, 0.2f, 0.2f);

    float distTraveled = 0.0f;
    for (int step = 0; step < maxSteps; ++step)
    {
        float3 currPos = rayStartPos + distTraveled * rayFwdNormal;

        float distToClosest = SdfMap(currPos);

        // float3 diffuseColor;
        // float distToClosest = SdfMapWithColor(currPos, diffuseColor);

        // Hit
        if (distToClosest < minHitDistance)
        {
            // Calculate Normal
            float3 N = SdfNormalTetra(currPos);

            /** Diffuse Lighting
            // Calculate Diffuse color
            float3 diffuseColor = GetWeightedColor(currPos, missingColor, N);

            // Shading
            SurfaceData surf = MakeDefaultSurfaceData();
            surf.Albedo = diffuseColor.rgb;
            surf.Normal = normalize(N);

            float3 totalLight;

            CALC_TOTAL_DIFFUSE_LIGHT(totalLight, surf, currPos);

            float3 color = saturate(totalLight);
            */
            SurfaceData surf = GetWeightedSurfaceData(currPos, N);

            float3 directLighting = float3(0.f, 0.f, 0.f); // Result

            CALC_TOTAL_PBR_LIGHT(directLighting, surf, currPos);

            float3 ambient = float3(0.02, 0.02, 0.02) * surf.Albedo.rgb * surf.AO;

            float3 color = ambient + directLighting + surf.Emission; 
            color = ACESFilm(color);
            color = pow(color, 1.0/2.2); // Gamma correction

            if (engineConstants.debugInt == 1)
            {
                color = surf.Albedo;
            }
            else if (engineConstants.debugInt == 2)
            {
                color = EncodeXYZToRGB(surf.Normal);
            }
            else if (engineConstants.debugInt == 3)
            {
                color = float3(0.f, surf.Roughness, surf.Metallic);
            }
            else if (engineConstants.debugInt == 4)
            {
                color = float3(surf.AO, surf.AO, surf.AO);
            }

            return float4(color, distTraveled); // return color + distance
        }

        // Miss
        if (distToClosest > maxTraceDistance)
        {
            return float4(missingColor, INFINITY_DIST); 
        }
        distTraveled += distToClosest;
    }

    // Miss
    return float4(missingColor, INFINITY_DIST); 
}

//-------------------------------------------------------------------------------------------
[numthreads(THREADS_PER_GROUP_SIZE, THREADS_PER_GROUP_SIZE, 1)]
void ComputeMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    ConstantBuffer<EngineConstants>     engineConstants = ResourceDescriptorHeap[renderResources.engineConstantsIndex];
    ConstantBuffer<CameraConstants>     cameraConstants = ResourceDescriptorHeap[renderResources.cameraConstantsIndex];
    ConstantBuffer<ModelConstants>      modelConstants = ResourceDescriptorHeap[renderResources.modelConstantsIndex];
    ConstantBuffer<LightConstants>      lightConstants = ResourceDescriptorHeap[renderResources.lightConstantsIndex];
    ConstantBuffer<PerFrameConstants>   perFrameConstants = ResourceDescriptorHeap[renderResources.perFrameConstantsIndex];

    StructuredBuffer<SdfShape> sdfShapes = ResourceDescriptorHeap[renderResources.inputSdfShapesIndex];
    RWTexture2D<float4> outputTex = ResourceDescriptorHeap[renderResources.outputTextureIndex];
    ConstantBuffer<SdfRayMarchingConstants>   sdfConstants = ResourceDescriptorHeap[renderResources.rayMarchingConstantsIndex];
    
    int2 pixelCoord = dispatchThreadID.xy;
    int2 screenSize = int2(sdfConstants.screenWidth, sdfConstants.screenHeight);
    if (any(pixelCoord >= screenSize))
        return;

    float2 uv = float2(pixelCoord) / float2(screenSize);
    float4 clipSpacePos = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 1.0f, 1.0f); // Near plane z = 0, Far Plane z = 1
    float4 worldSpacePos = mul(cameraConstants.clipToWorldTransform, clipSpacePos);
    worldSpacePos /= worldSpacePos.w;

    const float3 rayStartPos = cameraConstants.cameraWorldPosition;
    const float3 rayFwdNormal = normalize(worldSpacePos.xyz - rayStartPos);


    float4 marchRes = RayMarch(rayStartPos, rayFwdNormal);


    outputTex[pixelCoord] = float4(marchRes.xyz, 1.f); // Opaque
    if (renderResources.outputDepthIndex != INVALID_INDEX)
    {
        RWTexture2D<float> outputDepthTex = ResourceDescriptorHeap[renderResources.outputDepthIndex];

        float4 pixelWorldPos = float4((rayStartPos + (marchRes.w * rayFwdNormal)) , 1.f);
        float4 cameraSpacePosition = mul(cameraConstants.worldToCameraTransform, pixelWorldPos);
	    float4 renderSpacePosition = mul(cameraConstants.cameraToRenderTransform, cameraSpacePosition);
	    float4 clipSpacePosition = mul(cameraConstants.renderToClipTransform, renderSpacePosition);

        float depth = clipSpacePosition.z / clipSpacePosition.w;

        outputDepthTex[pixelCoord] = depth;
    }
}


// float2 sminCubicWithMixFactor(float a, float b, float k)
// {
//     float h = 1.0 - min( abs(a-b)/(6.0*k), 1.0 );
//     float w = h*h*h;
//     float m = w*0.5;
//     float s = w*k; 
//     return (a<b) ? float2(a-s,m) : float2(b-s,1.0-m);
// }


// float SdfMapWithColor(float3 p, out float3 outColor)
// {
//     StructuredBuffer<SdfShape> sdfShapes = ResourceDescriptorHeap[renderResources.inputSdfShapesIndex];
//     ConstantBuffer<SdfRayMarchingConstants>   sdfConstants = ResourceDescriptorHeap[renderResources.rayMarchingConstantsIndex];

//     const int numOfShapes = sdfConstants.numOfShapes;
//     const float toleranceK = sdfConstants.toleranceK;

//     float resDist = INFINITY_DIST;
//     float3 resColor = float3(0,0,0);
//     bool first = true;

//     for (int i = 0; i < numOfShapes; ++i)
//     {
//         float d = sdfValueFromShape(p, sdfShapes[i]);
//         float3 c = sdfShapes[i].color.rgb;

//         if (first)
//         {
//             resDist = d;
//             resColor = c;
//             first = false;
//         }
//         else
//         {
//             float2 smin = sminCubicWithMixFactor(resDist, d, toleranceK);
//             float blend = smin.y;
//             // lerp(a,b,t) = a + t * (b-a)
//             resColor = lerp(c, resColor, blend);
//             resDist = smin.x;
//         }
//     }
//     outColor = resColor;
//     return resDist;
// }
