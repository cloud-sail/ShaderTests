#pragma once

struct UnlitRenderResources
{
    uint diffuseTextureIndex;
    uint diffuseSamplerIndex;
    
    uint cameraConstantsIndex;
    uint modelConstantsIndex;
};

struct DiffuseRenderResources
{
    uint diffuseTextureIndex;
    uint diffuseSamplerIndex;
    
    uint cameraConstantsIndex;
    uint modelConstantsIndex;
    uint lightConstantsIndex;
};

struct BlinnPhongRenderResources
{
    uint diffuseTextureIndex;
    uint normalTextureIndex;
    uint specGlossEmitTextureIndex;
    
    uint engineConstantsIndex;
    uint cameraConstantsIndex;
    uint modelConstantsIndex;
    uint lightConstantsIndex;
};

struct SkyboxRenderResources
{
    uint cubeMapTextureIndex;

    uint cameraConstantsIndex;
    uint modelConstantsIndex;
};

struct PBRRenderResources
{
    uint albedoTextureIndex; 
    uint metallicRoughnessTextureIndex;
    uint normalTextureIndex;
    uint occlusionTextureIndex;
    uint emissiveTextureIndex;

    uint samplerIndex;
    
    uint engineConstantsIndex;
    uint cameraConstantsIndex;
    uint modelConstantsIndex;
    uint lightConstantsIndex;
    /*
    uint materialConstantsIndex; // #Todo
    struct MaterialConstants
    {
        float4 baseColorFactor;
        float metallicFactor;
        float roughnessFactor;
        float3 emissiveFactor;
        float normalScale;
        float occlusionStrength;
    };
    */
};
