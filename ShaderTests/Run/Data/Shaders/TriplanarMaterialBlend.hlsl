#include "Common/Utils.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/Resources.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/TriplanarUtils.hlsli"
#include "Common/ToneMapping.hlsli"

//------------------------------------------------------------------------------------------------

struct vs_input_t
{
    float3 modelPosition : POSITION;
    float3 modelNormal   : NORMAL;
    uint2  matdens      : MATDENS; // x: materialID, y: density
    // uint materialID : MATERIALID;
    // uint density    : DENSITY;
};

//------------------------------------------------------------------------------------------------

struct v2p_t
{
    float4 clipPosition  : SV_Position;
    float3 worldPos      : WORLD_POSITION;
    float3 worldNormal   : WORLD_NORMAL;
    nointerpolation uint materialID : MATERIALID;
    nointerpolation uint density    : DENSITY;
};


struct Material
{
    uint albedoTextureIndex;
    uint metallicRoughnessTextureIndex;
    uint normalTextureIndex;
    uint occlusionTextureIndex;
    uint emissiveTextureIndex;

    uint samplerIndex;
};

struct TriplanarMaterialBlendRenderResources
{
    uint materialBufferIndex; // StructuredBuffer<TriplanarMaterial> materialBuffer;

    float uvScale;
    float blendSharpness;

    uint engineConstantsIndex;
    uint cameraConstantsIndex;
    uint modelConstantsIndex;
    uint lightConstantsIndex;
    uint perFrameConstantsIndex;
};

//----------------------------------------------------------------------------------------------------
ConstantBuffer<TriplanarMaterialBlendRenderResources> renderResources : register(b0);

v2p_t VertexMain(vs_input_t input)
{
    ConstantBuffer<CameraConstants> cameraConstants = ResourceDescriptorHeap[renderResources.cameraConstantsIndex];
    ConstantBuffer<ModelConstants> modelConstants = ResourceDescriptorHeap[renderResources.modelConstantsIndex];

    float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(modelConstants.modelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(cameraConstants.worldToCameraTransform, worldPosition);
	float4 renderPosition = mul(cameraConstants.cameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(cameraConstants.renderToClipTransform, renderPosition);

    float4 worldNormal = mul(modelConstants.modelToWorldTransform, float4(input.modelNormal, 0.0f));

    v2p_t v2p;
    v2p.clipPosition = clipPosition;
    v2p.worldPos     = worldPosition.xyz;
    v2p.worldNormal  = worldNormal.xyz;
    v2p.materialID   = input.matdens.x;
    v2p.density      = input.matdens.y;
    return v2p;
}

//---------------------------------------------------------------------------------------------
enum VertexID { 
    FIRST = 0, 
    SECOND = 1, 
    THIRD = 2 
};

float4 PixelMain(
    v2p_t input,
    float3 bary : SV_Barycentrics
    // nointerpolation uint materialID : MATERIALID,
    // nointerpolation uint density    : DENSITY
) : SV_Target0
{
    ConstantBuffer<EngineConstants> engineConstants = ResourceDescriptorHeap[renderResources.engineConstantsIndex];
    ConstantBuffer<CameraConstants> cameraConstants = ResourceDescriptorHeap[renderResources.cameraConstantsIndex];
    ConstantBuffer<ModelConstants> modelConstants = ResourceDescriptorHeap[renderResources.modelConstantsIndex];
    ConstantBuffer<LightConstants> lightConstants = ResourceDescriptorHeap[renderResources.lightConstantsIndex];

    uint materialID0 = GetAttributeAtVertex(input.materialID, VertexID::FIRST);
    uint materialID1 = GetAttributeAtVertex(input.materialID, VertexID::SECOND);
    uint materialID2 = GetAttributeAtVertex(input.materialID, VertexID::THIRD);

    uint density0 = GetAttributeAtVertex(input.density, VertexID::FIRST);
    uint density1 = GetAttributeAtVertex(input.density, VertexID::SECOND);
    uint density2 = GetAttributeAtVertex(input.density, VertexID::THIRD);

    StructuredBuffer<Material> materialBuffer = ResourceDescriptorHeap[renderResources.materialBufferIndex];;

    // Optimize for same material on 3 vertex
    if (materialID0 == materialID1 && materialID1 == materialID2)
    {
        Material mat = materialBuffer[materialID0];
        SamplerState samp = SamplerDescriptorHeap[mat.samplerIndex];

        float3 N = normalize(input.worldNormal);

        float4 albedoTexel = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                            ResourceDescriptorHeap[mat.albedoTextureIndex], samp);
        float2 metallicRoughness = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                            ResourceDescriptorHeap[mat.metallicRoughnessTextureIndex], samp).bg;
        float occlusion = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                            ResourceDescriptorHeap[mat.occlusionTextureIndex], samp).r;
        float3 emissive = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                            ResourceDescriptorHeap[mat.emissiveTextureIndex], samp).rgb;

        float3 pixelNormalWorldSpace = SampleTriplanarNormal(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                                             ResourceDescriptorHeap[mat.normalTextureIndex], samp);


        float4 modelColor = modelConstants.modelColor;
        float4 diffuseColor = albedoTexel * modelColor;
        clip(diffuseColor.a < 0.01f);

        SurfaceData surf = MakeDefaultSurfaceData();
        surf.Albedo    = diffuseColor.rgb;
        surf.Normal    = pixelNormalWorldSpace;
        surf.Metallic  = metallicRoughness.x;
        surf.Roughness = metallicRoughness.y;

        float3 directLighting = float3(0.f, 0.f, 0.f);
        CALC_TOTAL_PBR_LIGHT(directLighting, surf, input.worldPos);

        float3 ambient = float3(0.02, 0.02, 0.02) * diffuseColor.rgb * occlusion;
        float3 color = ambient + directLighting + emissive;
        color = ACESFilm(color);
        color = pow(color, 1.0/2.2);

        return float4(color, 1.0);
    }
    else
    {
        // Blend multiple material
        float3 densities = float3(density0, density1, density2);
        float3 matWeights = bary * densities;

        float sumWeight = matWeights.x + matWeights.y + matWeights.z;
        matWeights = (sumWeight > 0.0) ? matWeights / sumWeight : bary;

        Material mat0 = materialBuffer[materialID0];
        Material mat1 = materialBuffer[materialID1];
        Material mat2 = materialBuffer[materialID2];

        SamplerState samp0 = SamplerDescriptorHeap[mat0.samplerIndex];
        SamplerState samp1 = SamplerDescriptorHeap[mat1.samplerIndex];
        SamplerState samp2 = SamplerDescriptorHeap[mat2.samplerIndex];

        float3 N = normalize(input.worldNormal);

        float4 albedo0 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                         ResourceDescriptorHeap[mat0.albedoTextureIndex], samp0);
        float4 albedo1 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                         ResourceDescriptorHeap[mat1.albedoTextureIndex], samp1);
        float4 albedo2 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                         ResourceDescriptorHeap[mat2.albedoTextureIndex], samp2);

        float2 mr0 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                     ResourceDescriptorHeap[mat0.metallicRoughnessTextureIndex], samp0).bg;
        float2 mr1 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                     ResourceDescriptorHeap[mat1.metallicRoughnessTextureIndex], samp1).bg;
        float2 mr2 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                     ResourceDescriptorHeap[mat2.metallicRoughnessTextureIndex], samp2).bg;

        float occ0 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                     ResourceDescriptorHeap[mat0.occlusionTextureIndex], samp0).r;
        float occ1 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                     ResourceDescriptorHeap[mat1.occlusionTextureIndex], samp1).r;
        float occ2 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                     ResourceDescriptorHeap[mat2.occlusionTextureIndex], samp2).r;

        float3 emi0 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                      ResourceDescriptorHeap[mat0.emissiveTextureIndex], samp0).rgb;
        float3 emi1 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                      ResourceDescriptorHeap[mat1.emissiveTextureIndex], samp1).rgb;
        float3 emi2 = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                      ResourceDescriptorHeap[mat2.emissiveTextureIndex], samp2).rgb;

        float3 normal0 = SampleTriplanarNormal(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                               ResourceDescriptorHeap[mat0.normalTextureIndex], samp0);
        float3 normal1 = SampleTriplanarNormal(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                               ResourceDescriptorHeap[mat1.normalTextureIndex], samp1);
        float3 normal2 = SampleTriplanarNormal(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
                                               ResourceDescriptorHeap[mat2.normalTextureIndex], samp2);

        float4 modelColor = modelConstants.modelColor;

        // simple blending
        float4 albedoTexel = albedo0 * matWeights.x + albedo1 * matWeights.y + albedo2 * matWeights.z;
        float2 metallicRoughness = mr0 * matWeights.x + mr1 * matWeights.y + mr2 * matWeights.z;
        float occlusion = occ0 * matWeights.x + occ1 * matWeights.y + occ2 * matWeights.z;
        float3 emissive = emi0 * matWeights.x + emi1 * matWeights.y + emi2 * matWeights.z;
        float3 pixelNormalWorldSpace = normalize(normal0 * matWeights.x + normal1 * matWeights.y + normal2 * matWeights.z);
        
        float4 diffuseColor = albedoTexel * modelColor;
        clip(diffuseColor.a < 0.01f);

        SurfaceData surf = MakeDefaultSurfaceData();
        surf.Albedo    = diffuseColor.rgb;
        surf.Normal    = pixelNormalWorldSpace;
        surf.Metallic  = metallicRoughness.x;
        surf.Roughness = metallicRoughness.y;

        float3 directLighting = float3(0.f, 0.f, 0.f);
        CALC_TOTAL_PBR_LIGHT(directLighting, surf, input.worldPos);

        float3 ambient = float3(0.02, 0.02, 0.02) * diffuseColor.rgb * occlusion;
        float3 color = ambient + directLighting + emissive;
        color = ACESFilm(color);
        color = pow(color, 1.0/2.2);

        return float4(color, 1.0);
    }
}