#include "Common/Utils.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/Resources.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/TriplanarUtils.hlsli"

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


struct TriplanarRenderResources
{ 
    uint cameraConstantsIndex;
    uint modelConstantsIndex;
    uint lightConstantsIndex;

    float uvScale;
    float blendSharpness;

    // Currently use Diffuse Shading, Later use pbr
    uint diffuseTextureIndex;
    uint diffuseSamplerIndex;
};


//----------------------------------------------------------------------------------------------------
ConstantBuffer<TriplanarRenderResources> renderResources : register(b0);

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

    Texture2D<float4> diffuseTexture = ResourceDescriptorHeap[renderResources.diffuseTextureIndex];
	SamplerState diffuseSampler = SamplerDescriptorHeap[renderResources.diffuseSamplerIndex];

    
    float3 N = normalize(input.worldNormal.xyz);

    float4 diffuseTexel = SampleTriplanar(input.worldPos, N, renderResources.uvScale, renderResources.blendSharpness,
        diffuseTexture, diffuseSampler);

	float4 surfaceColor = input.color; // from vertex data
	float4 modelColor = modelConstants.modelColor;
    float4 diffuseColor = diffuseTexel * surfaceColor * modelColor;

    SurfaceData surf = MakeDefaultSurfaceData();
    surf.Albedo = diffuseColor.rgb;
    surf.Normal = N;

    float3 totalLight = float3(0.f, 0.f, 0.f); // Result

	CALC_TOTAL_DIFFUSE_LIGHT(totalLight, surf, input.worldPos);

    float3 finalRGB = saturate(totalLight);

    float4 finalColor = float4(finalRGB, diffuseColor.a);

	clip(finalColor.a - 0.01f);
	return finalColor;
}