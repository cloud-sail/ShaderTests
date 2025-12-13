#include "Common/Utils.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/Resources.hlsli"
#include "Common/StaticSampler.hlsli"


struct vs_input_t
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipPosition : SV_POSITION;
    float3 sampleDir : POSITION; // for cubemap sampling
};
ConstantBuffer<SkyboxRenderResources> renderResources : register(b0);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
    ConstantBuffer<CameraConstants> cameraConstants = ResourceDescriptorHeap[renderResources.cameraConstantsIndex];

	v2p_t v2p;

	float4 modelSpacePosition = float4(input.modelPosition, 1);
    v2p.sampleDir = mul(cameraConstants.cameraToRenderTransform, float4(input.modelPosition, 0.f)).xyz; // IMPORTANT

    float4 worldSpacePosition = modelSpacePosition;
    worldSpacePosition.xyz += cameraConstants.cameraWorldPosition; // follow camera, do not need model constants

	float4 cameraSpacePosition =  mul(cameraConstants.worldToCameraTransform, worldSpacePosition);
	float4 renderSpacePosition = mul(cameraConstants.cameraToRenderTransform, cameraSpacePosition);
	float4 clipPosition = mul(cameraConstants.renderToClipTransform, renderSpacePosition);

	v2p.clipPosition = clipPosition.xyww; // depth = 1.f 
	return v2p;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target
{
	TextureCube cubeMapTexture = ResourceDescriptorHeap[renderResources.cubeMapTextureIndex];
    return cubeMapTexture.Sample(s_linearWrap, input.sampleDir);
}