#include "Common/Utils.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/Resources.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/ToneMapping.hlsli"
#include "Common/StaticSampler.hlsli"

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

struct TestPBRRenderResources
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

    uint radianceTextureIndex;
	uint irradianceTextureIndex;
	uint brdfLutTextureIndex;

    float debugMetallic;
	float debugRoughness;
	float uvScale;
};

//----------------------------------------------------------------------------------------------------
ConstantBuffer<TestPBRRenderResources> renderResources : register(b0);

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
    ConstantBuffer<EngineConstants> endgineConstants = ResourceDescriptorHeap[renderResources.engineConstantsIndex];

	SamplerState samp = SamplerDescriptorHeap[renderResources.samplerIndex];

    float2 uvCoords = input.uv / renderResources.uvScale;
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

	if (endgineConstants.debugInt == 1)
	{
		surf.Albedo = float3(0.9, 0.9, 0.9);
		surf.Normal = surfaceNormalWorldSpace;
		surf.Metallic = renderResources.debugMetallic;
		surf.Roughness = renderResources.debugRoughness;
	}

    float3 directLighting = float3(0.f, 0.f, 0.f); // Result

	CALC_TOTAL_PBR_LIGHT(directLighting, surf, input.worldPos);


	float3 f0 = lerp(float3(0.04,0.04,0.04), surf.Albedo, surf.Metallic);

	float3 viewDir = normalize(cameraConstants.cameraWorldPosition.xyz - input.worldPos); // input

	float3 N = normalize(surf.Normal);
    float3 V = normalize(viewDir);
	float NdotV = saturate(dot(N, V));

	float3 F = FresnelSchlickWithRoughness(NdotV, f0, surf.Roughness);
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= (1.0 - surf.Metallic);

	TextureCube<float4> irradianceTexture = ResourceDescriptorHeap[renderResources.irradianceTextureIndex];
	TextureCube<float4> preFilterTexture = ResourceDescriptorHeap[renderResources.radianceTextureIndex];
    Texture2D<float2> brdfLutTexture = ResourceDescriptorHeap[renderResources.brdfLutTextureIndex];

	float3 irradianceSampleDir = mul(cameraConstants.cameraToRenderTransform, float4(surf.Normal, 0.f)).xyz;
	float3 irradiance = irradianceTexture.Sample(s_linearClamp, irradianceSampleDir).rgb;
	// float3 irradiance = irradianceTexture.Sample(samp, surf.Normal).rgb;
	float3 diffuseIBL = kD * irradiance * surf.Albedo.xyz / kPi; // /kPi is different

	float3 R = reflect(-V, N);

	const float MAX_REFLECTION_LOD = 6.0;
	float mipLevel = surf.Roughness * (MAX_REFLECTION_LOD - 1.0);
	float3 prefilterSampleDir = mul(cameraConstants.cameraToRenderTransform, float4(R, 0.f)).xyz;
	float3 prefilteredColor  = preFilterTexture.SampleLevel(s_linearClamp, prefilterSampleDir, mipLevel).xyz;
	// float3 prefilteredColor  = preFilterTexture.SampleLevel(samp, R, mipLevel).xyz;
    float2 envBRDF = brdfLutTexture.Sample(s_linearClamp, float2(NdotV, 1 - surf.Roughness)).xy; // dds lut: 1 - y

	float3 specularIBL = prefilteredColor  * (F * envBRDF.x  + envBRDF.y); // F?

	float3 ambient = (diffuseIBL + specularIBL) * surf.AO;
	// ambient = (diffuseIBL) * surf.AO;
	if (endgineConstants.debugInt == 2)
	{
    	ambient = float3(0.02, 0.02, 0.02) * diffuseColor.rgb * occlusion;
	}


    float3 color = ambient + directLighting + emissive; 


    color = ACESFilm(color);
    color = pow(color, 1.0/2.2); // Gamma correction

	return float4(color, 1.0); 
}
