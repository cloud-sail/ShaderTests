#include "Game/Prop.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

Prop::Prop(Game* owner)
	: Entity(owner)
{
}

void Prop::Update(float deltaSeconds)
{
	m_orientation.m_yawDegrees += deltaSeconds * m_angularVelocity.m_yawDegrees;
	m_orientation.m_pitchDegrees += deltaSeconds * m_angularVelocity.m_pitchDegrees;
	m_orientation.m_rollDegrees += deltaSeconds * m_angularVelocity.m_rollDegrees;
}

void Prop::Render() const
{
	g_theRenderer->SetModelConstants(GetModelToWorldTransform(), m_color);

	// resource settings
	UnlitRenderResources resources;
	resources.diffuseTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(m_texture, DefaultTexture::WhiteOpaque2D);
	resources.diffuseSamplerIndex = g_theRenderer->GetDefaultSamplerIndex(SamplerMode::POINT_CLAMP);
	resources.cameraConstantsIndex = g_theRenderer->GetCurrentCameraConstantsIndex();
	resources.modelConstantsIndex = g_theRenderer->GetCurrentModelConstantsIndex();

	g_theRenderer->SetGraphicsBindlessResources(sizeof(UnlitRenderResources), &resources);

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	//g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetRenderTargetFormats();
	g_theRenderer->DrawVertexArray(m_vertexes);
}
