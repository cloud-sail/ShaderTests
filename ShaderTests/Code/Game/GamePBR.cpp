#include "Game/GamePBR.hpp"
#include "Game/SpectatorCamera.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"

#include "ThirdParty/imgui/imgui.h"


GamePBR::GamePBR()
{
	constexpr float CELL_ASPECT = 0.9f;
	constexpr float TEXT_HEIGHT = 0.2f;
	constexpr float ORIGIN_OFFSET = 0.15f;
	DebugAddWorldBasis(Mat44(), -1.f);
	DebugAddWorldText("x - forward", Mat44(Vec3(0.f, -1.f, 0.f), Vec3(1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(ORIGIN_OFFSET, 0.f, ORIGIN_OFFSET)), TEXT_HEIGHT, -1.f, CELL_ASPECT, Vec2::ZERO, Rgba8::RED);
	DebugAddWorldText("y - left", Mat44(Vec3(-1.f, 0.f, 0.f), Vec3(0.f, -1.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(0.f, ORIGIN_OFFSET, ORIGIN_OFFSET)), TEXT_HEIGHT, -1.f, CELL_ASPECT, Vec2(1.f, 0.f), Rgba8::GREEN);
	DebugAddWorldText("z - up", Mat44(Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, -ORIGIN_OFFSET, ORIGIN_OFFSET)), TEXT_HEIGHT, -1.f, CELL_ASPECT, Vec2(0.f, 1.f), Rgba8::BLUE);


	m_spectator = new SpectatorCamera();
	m_cursorMode = CursorMode::FPS;

	LoadModel();

}

GamePBR::~GamePBR()
{
	delete m_spectator;
	m_spectator = nullptr;

	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;
}

void GamePBR::Update()
{
	UpdateDeveloperCheats();
	ShowCommonImGuiWindow();
	ShowGameModeImGuiWindow();

	if (g_theInput->WasKeyJustPressed(KEYCODE_F))
	{
		ToggleCursorMode();
	}
	if (g_isDebugDraw)
	{
		DebugDrawLights();
	}

	m_spectator->Update();
}

void GamePBR::Render() const
{

	g_theRenderer->BeginCamera(m_spectator->m_camera);
	// World-space drawing
	RenderModel();
	g_theRenderer->EndCamera(m_spectator->m_camera);
	DebugRenderWorld(m_spectator->m_camera);

	//g_theRenderer->BeginCamera(m_screenCamera);
	//// Render UI
	//g_theRenderer->EndCamera(m_screenCamera);
	//DebugRenderScreen(m_screenCamera);
}

void GamePBR::Reset()
{
	m_uvScale = 1.f;
	m_blendSharpness = 1.f;
}

void GamePBR::OnWindowResized()
{
	m_spectator->RefreshAspectRatio();
}

void GamePBR::LoadModel()
{
	// Load model material
	m_shader = g_theRenderer->CreateOrGetShader(ShaderConfig("Data/Shaders/PBR"), VertexType::VERTEX_PCUTBN);


	m_sampler = SamplerMode::BILINEAR_WRAP;
	m_albedoTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_albedo.png");
	m_metallicRoughnessTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_arm.png");
	m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_normal-ogl.png");
	m_occlusionTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_arm.png");
	m_emissiveTexture = nullptr;


	// Load model shape
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(1 * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(1 * sizeof(unsigned int));

	std::vector<Vertex_PCUTBN> verts;
	std::vector<unsigned int> indexes;

	AddVertsForSphere3D(verts, indexes, Vec3(0.f, 0.f, 2.f), 1.f);
	AddVertsForAABB3D(verts, indexes, AABB3(Vec3(2.f, 2.f, 0.f), Vec3(3.f, 3.f, 1.f)));

	g_theRenderer->CopyCPUToGPU(verts.data(), static_cast<unsigned int>(verts.size()) * m_vertexBuffer->GetStride(), m_vertexBuffer);
	g_theRenderer->CopyCPUToGPU(indexes.data(), static_cast<unsigned int>(indexes.size()) * m_indexBuffer->GetStride(), m_indexBuffer);

}

void GamePBR::RenderModel() const
{
	PBRRenderResources resources;
	resources.engineConstantsIndex = g_theRenderer->GetCurrentEngineConstantsIndex();
	resources.cameraConstantsIndex = g_theRenderer->GetCurrentCameraConstantsIndex();
	resources.modelConstantsIndex = g_theRenderer->GetCurrentModelConstantsIndex();
	resources.lightConstantsIndex = g_theRenderer->GetCurrentLightConstantsIndex();

	resources.samplerIndex = g_theRenderer->GetDefaultSamplerIndex(m_sampler);

	resources.albedoTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(m_albedoTexture, DefaultTexture::WhiteOpaque2D);
	resources.metallicRoughnessTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(m_metallicRoughnessTexture, DefaultTexture::DefaultOcclusionRoughnessMetalnessMap);
	resources.normalTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(m_normalTexture, DefaultTexture::DefaultNormalMap);
	resources.occlusionTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(m_occlusionTexture, DefaultTexture::DefaultOcclusionRoughnessMetalnessMap);
	resources.emissiveTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(m_emissiveTexture, DefaultTexture::BlackOpaque2D);


	g_theRenderer->SetGraphicsBindlessResources(sizeof(PBRRenderResources), &resources);

	g_theRenderer->BindShader(m_shader);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetRenderTargetFormats();

	g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, m_indexBuffer->GetCount());
}

void GamePBR::ShowGameModeImGuiWindow()
{
	//if (ImGui::Begin("Triplanar Mapping"))
	//{
	//	if (ImGui::Button("Reset Scene"))
	//	{
	//		Reset();
	//	}

	//	ImGui::SliderFloat("UV Scale", &m_uvScale, 0.1f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
	//	ImGui::SliderFloat("Blend Sharpness", &m_blendSharpness, 0.1f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
	//}

	//ImGui::End();
}
