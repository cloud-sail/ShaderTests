#include "Common/Utils.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/Resources.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/ToneMapping.hlsli"

//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipPosition		: SV_Position;
	float4 color			: COLOR;
	float2 uv				: TEXCOORD;
	float3 worldPos			: WORLD_POSITION;
	float4 worldTangent		: WORLD_TANGENT;
	float4 worldBitangent	: WORLD_BITANGENT;
	float4 worldNormal		: WORLD_NORMAL;
	float4 modelTangent		: MODEL_TANGENT;
	float4 modelBitangent	: MODEL_BITANGENT;
	float4 modelNormal		: MODEL_NORMAL;
};

//----------------------------------------------------------------------------------------------------
ConstantBuffer<PBRRenderResources> renderResources : register(b0);

v2p_t VertexMain(vs_input_t input)
{
    ConstantBuffer<CameraConstants> cameraConstants = ResourceDescriptorHeap[renderResources.cameraConstantsIndex];
    ConstantBuffer<ModelConstants> modelConstants = ResourceDescriptorHeap[renderResources.modelConstantsIndex];

	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(modelConstants.modelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(cameraConstants.worldToCameraTransform, worldPosition);
	float4 renderPosition = mul(cameraConstants.cameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(cameraConstants.renderToClipTransform, renderPosition);

	float4 worldTangent = mul(modelConstants.modelToWorldTransform, float4(input.modelTangent, 0.0f));
	float4 worldBitangent = mul(modelConstants.modelToWorldTransform, float4(input.modelBitangent, 0.0f));
	float4 worldNormal = mul(modelConstants.modelToWorldTransform, float4(input.modelNormal, 0.0f));

	v2p_t v2p;
	v2p.clipPosition = clipPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldPos = worldPosition.xyz;
	v2p.worldTangent = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal = worldNormal;
	v2p.modelTangent = float4(input.modelTangent, 0.0f);
	v2p.modelBitangent = float4(input.modelBitangent, 0.0f);
	v2p.modelNormal = float4(input.modelNormal, 0.0f);
	return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    ConstantBuffer<CameraConstants> cameraConstants = ResourceDescriptorHeap[renderResources.cameraConstantsIndex];
    ConstantBuffer<ModelConstants> modelConstants = ResourceDescriptorHeap[renderResources.modelConstantsIndex];
    ConstantBuffer<LightConstants> lightConstants = ResourceDescriptorHeap[renderResources.lightConstantsIndex];

	SamplerState samp = SamplerDescriptorHeap[renderResources.samplerIndex];

    float2 uvCoords = input.uv;
    float4 albedoTexel = GetSafeAlbedo(renderResources.albedoTextureIndex, samp, uvCoords);
    float2 metalicRoughness = GetSafeMetallicRoughness(renderResources.metallicRoughnessTextureIndex, samp, uvCoords);
    float3 pixelNormalTBNSpace = GetSafeNormal(renderResources.normalTextureIndex, samp, uvCoords);
    float occlusion = GetSafeOcclusion(renderResources.occlusionTextureIndex , samp, uvCoords);
    float3 emissive = GetSafeEmissive(renderResources.emissiveTextureIndex , samp, uvCoords);
    
	float4 surfaceColor = input.color; 
	float4 modelColor = modelConstants.modelColor;
    float4 diffuseColor = albedoTexel * surfaceColor * modelColor;
    clip(diffuseColor.a < 0.01f);


	float3 surfaceNormalWorldSpace = normalize(input.worldNormal.xyz);
	float3 surfaceTangentWorldSpace = normalize(input.worldTangent.xyz - dot(input.worldTangent.xyz, surfaceNormalWorldSpace) * surfaceNormalWorldSpace);
	float3 surfaceBitangentWorldSpace = cross(surfaceNormalWorldSpace, surfaceTangentWorldSpace); // reset the handness?

	float3x3 tbnToWorld = float3x3(surfaceTangentWorldSpace, surfaceBitangentWorldSpace, surfaceNormalWorldSpace);
	
	float3 pixelNormalWorldSpace = mul(pixelNormalTBNSpace, tbnToWorld);

    SurfaceData surf = MakeDefaultSurfaceData();
    surf.Albedo = diffuseColor.rgb;
    surf.Normal = pixelNormalWorldSpace;
    surf.Metallic = metalicRoughness.x;
    surf.Roughness = metalicRoughness.y;

    float3 directLighting = float3(0.f, 0.f, 0.f); // Result

	CALC_TOTAL_PBR_LIGHT(directLighting, surf, input.worldPos);

    float3 ambient = float3(0.02, 0.02, 0.02) * diffuseColor.rgb * occlusion;

    // float3 color = ambient + directLighting + emissive; // + emissive + indirectDiffuse + indirectSpecular
    float3 color = ambient + directLighting + emissive; 
    color = ACESFilm(color);
    color = pow(color, 1.0/2.2); // Gamma correction

	return float4(color, 1.0); 
}
