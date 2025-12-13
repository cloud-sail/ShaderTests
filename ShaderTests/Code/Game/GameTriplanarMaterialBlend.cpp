#include "Game/GameTriplanarMaterialBlend.hpp"
#include "Game/SpectatorCamera.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "ThirdParty/imgui/imgui.h"

GameTriplanarMaterialBlend::GameTriplanarMaterialBlend()
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

	Initialize();

}

GameTriplanarMaterialBlend::~GameTriplanarMaterialBlend()
{
	delete m_spectator;
	m_spectator = nullptr;

	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	g_theRenderer->DestroyBuffer(m_materialBuffer);
	g_theRenderer->EnqueueDeferredRelease(m_materialBufferSRV);
}

void GameTriplanarMaterialBlend::Update()
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

	UpdateModel();
}

void GameTriplanarMaterialBlend::Render() const
{

	g_theRenderer->BeginCamera(m_spectator->m_camera);
	RenderModel();
	g_theRenderer->EndCamera(m_spectator->m_camera);
	DebugRenderWorld(m_spectator->m_camera);

	//g_theRenderer->BeginCamera(m_screenCamera);
	//// Render UI
	//g_theRenderer->EndCamera(m_screenCamera);
	//DebugRenderScreen(m_screenCamera);
}

void GameTriplanarMaterialBlend::Reset()
{
	m_uvScale = 1.f;
	m_blendSharpness = 1.f;
}

void GameTriplanarMaterialBlend::OnWindowResized()
{
	m_spectator->RefreshAspectRatio();
}

void GameTriplanarMaterialBlend::Initialize()
{
	m_shader = g_theRenderer->CreateOrGetShader(ShaderConfig("Data/Shaders/TriplanarMaterialBlend"), VertexType::VERTEX_PNMD);

	m_position = Vec3(2.f, 2.f, 2.f);

	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(1 * sizeof(Vertex_PNMD), sizeof(Vertex_PNMD));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(1 * sizeof(unsigned int));

	m_vertices.clear();
	m_indices.clear();

	m_vertices.push_back(Vertex_PNMD(Vec3(0.f, -0.5f, -0.5f), Vec3::FORWARD, 0, 255));
	m_vertices.push_back(Vertex_PNMD(Vec3(0.f, 0.5f, -0.5f), Vec3::FORWARD, 0, 255));
	m_vertices.push_back(Vertex_PNMD(Vec3(0.f, 0.5f, 0.5f), Vec3::FORWARD, 0, 255));
	m_vertices.push_back(Vertex_PNMD(Vec3(0.f, -0.5f, 0.5f), Vec3::FORWARD, 0, 255));

	m_indices.push_back(0);
	m_indices.push_back(1);
	m_indices.push_back(2);
	m_indices.push_back(0);
	m_indices.push_back(2);
	m_indices.push_back(3);

	//-----------------------------------------------------------------------------------------------
	constexpr int NUM_MATERIAL = 4;
	BufferInit initData;
	initData.m_size = NUM_MATERIAL * sizeof(GMaterial);
	m_materialBuffer = g_theRenderer->CreateBuffer(initData);

	m_materialBufferSRV = g_theRenderer->AllocateStructuredBufferSRV(*m_materialBuffer, sizeof(GMaterial), NUM_MATERIAL);

	std::vector<GMaterial> matData;
	matData.reserve(NUM_MATERIAL);

#define ADD_MAT(matName) \
    { \
        GMaterial mat; \
        SamplerMode sampler = SamplerMode::BILINEAR_WRAP; \
        Texture* albedoTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/" #matName "/" #matName "_albedo.png"); \
        Texture* metallicRoughnessTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/" #matName "/" #matName "_arm.png"); \
        Texture* normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/" #matName "/" #matName "_normal-ogl.png"); \
        Texture* occlusionTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/" #matName "/" #matName "_arm.png"); \
        Texture* emissiveTexture = nullptr; \
        mat.albedoTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(albedoTexture, DefaultTexture::CheckerboardMagentaBlack2D); \
        mat.metallicRoughnessTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(metallicRoughnessTexture, DefaultTexture::DefaultORMHMap); \
        mat.normalTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(normalTexture, DefaultTexture::DefaultNormalMap); \
        mat.occlusionTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(occlusionTexture, DefaultTexture::DefaultORMHMap); \
        mat.emissiveTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(emissiveTexture, DefaultTexture::BlackOpaque2D); \
        mat.samplerIndex = g_theRenderer->GetDefaultSamplerIndex(sampler); \
        matData.push_back(mat); \
    }

	ADD_MAT(fancy-scaled-gold);
	ADD_MAT(mud);
	ADD_MAT(rock-slab-wall);
	ADD_MAT(wispy-grass-meadow);

	g_theRenderer->UpdateBuffer(*m_materialBuffer, sizeof(GMaterial) * matData.size(), matData.data());

	//{
	//	GMaterial mat;

	//	SamplerMode sampler = SamplerMode::BILINEAR_WRAP;
	//	Texture* albedoTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_albedo.png");
	//	Texture* metallicRoughnessTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_arm.png");
	//	Texture* normalTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_normal-ogl.png");
	//	Texture* occlusionTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/fancy-scaled-gold/fancy-scaled-gold_arm.png");
	//	Texture* emissiveTexture = nullptr;

	//	mat.albedoTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(albedoTexture, DefaultTexture::CheckerboardMagentaBlack2D);
	//	mat.metallicRoughnessTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(metallicRoughnessTexture, DefaultTexture::DefaultOcclusionRoughnessMetalnessMap);
	//	mat.normalTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(normalTexture, DefaultTexture::DefaultNormalMap);
	//	mat.occlusionTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(occlusionTexture, DefaultTexture::DefaultOcclusionRoughnessMetalnessMap);
	//	mat.emissiveTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(emissiveTexture, DefaultTexture::BlackOpaque2D);

	//	mat.samplerIndex = g_theRenderer->GetDefaultSamplerIndex(sampler);

	//	matData.push_back(mat);
	//}
}

void GameTriplanarMaterialBlend::UpdateModel()
{
	Mat44 billBoardTransform = GetBillboardTransform(BillboardType::FULL_FACING,
		m_spectator->m_camera.GetCameraToWorldTransform(),
		m_position);

	m_orientation = billBoardTransform.GetEulerAngles();
	m_position = billBoardTransform.GetTranslation3D();
}

void GameTriplanarMaterialBlend::RenderModel() const
{
	{
		g_theRenderer->CopyCPUToGPU(m_vertices.data(), static_cast<unsigned int>(m_vertices.size()) * m_vertexBuffer->GetStride(), m_vertexBuffer);
		g_theRenderer->CopyCPUToGPU(m_indices.data(), static_cast<unsigned int>(m_indices.size()) * m_indexBuffer->GetStride(), m_indexBuffer);

		g_theRenderer->TransitionToGenericRead(*m_materialBuffer);

		Mat44 result = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		result.SetTranslation3D(m_position);
		g_theRenderer->SetModelConstants(result);

		TriplanarMaterialBlendResources res;


		res.materialBufferIndex = m_materialBufferSRV.m_index;


		res.uvScale = m_uvScale;
		res.blendSharpness = m_blendSharpness;

		res.engineConstantsIndex = g_theRenderer->GetCurrentEngineConstantsIndex();
		res.cameraConstantsIndex = g_theRenderer->GetCurrentCameraConstantsIndex();
		res.modelConstantsIndex = g_theRenderer->GetCurrentModelConstantsIndex();
		res.lightConstantsIndex = g_theRenderer->GetCurrentLightConstantsIndex();
		res.perFrameConstantsIndex = g_theRenderer->GetCurrentPerFrameConstantsIndex();


		g_theRenderer->SetGraphicsBindlessResources(sizeof(TriplanarMaterialBlendResources), &res);

		g_theRenderer->BindShader(m_shader);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->SetRenderTargetFormats();

		g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, m_indexBuffer->GetCount());
	}



	{
		//std::vector<Vertex_PCU> verts;
		//AddVertsForQuad3D(verts, Vec3(0.f, -0.5f, -0.5f), Vec3(0.f, 0.5f, -0.5f), Vec3(0.f, 0.5f, 0.5f), Vec3(0.f, -0.5f, 0.5f));

		//Mat44 result = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		//result.SetTranslation3D(m_position);
		//g_theRenderer->SetModelConstants(result);


		//// resource settings
		//UnlitRenderResources resources;
		//resources.diffuseTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(nullptr, DefaultTexture::WhiteOpaque2D);
		//resources.diffuseSamplerIndex = g_theRenderer->GetDefaultSamplerIndex(SamplerMode::POINT_CLAMP);
		//resources.cameraConstantsIndex = g_theRenderer->GetCurrentCameraConstantsIndex();
		//resources.modelConstantsIndex = g_theRenderer->GetCurrentModelConstantsIndex();

		//g_theRenderer->SetGraphicsBindlessResources(sizeof(UnlitRenderResources), &resources);


		//g_theRenderer->BindShader(nullptr);
		//g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		//g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		//g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		//g_theRenderer->SetRenderTargetFormats();
		//g_theRenderer->DrawVertexArray(verts);
	}

}

void GameTriplanarMaterialBlend::ShowGameModeImGuiWindow()
{
	if (ImGui::Begin("Triplanar Material Blending"))
	{
		if (ImGui::Button("Reset Scene"))
		{
			Reset();
		}

		ImGui::SliderFloat("UV Scale", &m_uvScale, 0.1f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("Blend Sharpness", &m_blendSharpness, 0.1f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);


		for (int i = 0; i < 4; ++i)
		{
			ImGui::PushID(i);
			ImGui::Text("Vertex %d", i);

			int materialID = static_cast<int>(m_vertices[i].m_materialID);
			int density = static_cast<int>(m_vertices[i].m_density);

			if (ImGui::InputInt("Material ID", &materialID, 1))
			{
				if (materialID < 0) materialID = 0;
				if (materialID > 255) materialID = 255;
				m_vertices[i].m_materialID = static_cast<uint8_t>(materialID);
			}
			if (ImGui::SliderInt("Density", &density, 0, 255))
			{
				m_vertices[i].m_density = static_cast<uint8_t>(density);
			}
			ImGui::Separator();
			ImGui::PopID();
		}
	}

	ImGui::End();
}
