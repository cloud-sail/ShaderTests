#include "Game/GameRayMarching.hpp"

#include "Game/SpectatorCamera.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Buffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Window/Window.hpp"


#include "ThirdParty/imgui/imgui.h"


SdfShape SdfShape::MakeSphere(Vec3 center, float radius, Rgba8 color /*= Rgba8::OPAQUE_WHITE*/)
{
	SdfShape result;

	result.m_type = SDF_SPHERE;
	result.m_data0 = Vec4(center.x, center.y, center.z, radius);
	color.GetAsFloats(result.m_color);

	return result;
}

//-----------------------------------------------------------------------------------------------

SdfShape GRMO_Sphere::GetShape() const
{
	SdfShape result = SdfShape::MakeSphere(m_position, m_radius, m_color);
	result.m_triAlbedoTexID = m_triAlbedoTexID;
	result.m_triMRTexID = m_triMRTexID;
	result.m_triNormalTexID = m_triNormalTexID;
	result.m_triOcclusionTexID = m_triOcclusionTexID;
	result.m_triEmissiveTexID = m_triEmissiveTexID;

	return result;
}




//-----------------------------------------------------------------------------------------------

static constexpr int INITIAL_SPHERE_COUNT = 2;
static constexpr float MIN_OBJECT_SPEED = 0.5f;
static constexpr float MAX_OBJECT_SPEED = 1.5f;
static constexpr float MIN_SPHERE_RADIUS = 0.5f;
static constexpr float MAX_SPHERE_RADIUS = 1.5f;
static constexpr float ACTIVITY_BOX_RADIUS = 5.f;


//-----------------------------------------------------------------------------------------------
GameRayMarching::GameRayMarching()
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


	m_fullScreenQuadShader = g_theRenderer->CreateOrGetShader(ShaderConfig("Data/Shaders/FullScreenQuad"), VertexType::VERTEX_NONE);
	m_fullScreenQuadWithDepthShader = g_theRenderer->CreateOrGetShader(ShaderConfig("Data/Shaders/FullScreenQuadWithDepth"), VertexType::VERTEX_NONE);
	m_diffuseShader = g_theRenderer->CreateOrGetShader(ShaderConfig("Data/Shaders/Diffuse"), VertexType::VERTEX_PCUTBN);

	ShaderConfig rayMarchingConfig;
	rayMarchingConfig.m_name = "Data/Shaders/SdfRayMarching";
	rayMarchingConfig.m_stages = SHADER_STAGE_CS;
	m_rayMarchingShader = g_theRenderer->CreateOrGetShader(rayMarchingConfig, VertexType::VERTEX_NONE);

	CreateRayMarchingConstants();


	m_triAlbedoTexs[0] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/mud/mud_albedo.png");
	//m_triAlbedoTexs[0] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	m_triMRTexs[0] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/mud/mud_arm.png");
	m_triNormalTexs[0] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/mud/mud_normal-ogl.png");
	m_triOcclusionTexs[0] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/mud/mud_arm.png");
	m_triEmissiveTexs[0] = nullptr;

	m_triAlbedoTexs[1] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/rock-slab-wall/rock-slab-wall_albedo.png");
	m_triMRTexs[1] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/rock-slab-wall/rock-slab-wall_arm.png");
	m_triNormalTexs[1] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/rock-slab-wall/rock-slab-wall_normal-ogl.png");
	m_triOcclusionTexs[1] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/rock-slab-wall/rock-slab-wall_arm.png");
	m_triEmissiveTexs[1] = nullptr;
	
	m_triAlbedoTexs[2] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/wispy-grass-meadow/wispy-grass-meadow_albedo.png");
	m_triMRTexs[2] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/wispy-grass-meadow/wispy-grass-meadow_arm.png");
	m_triNormalTexs[2] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/wispy-grass-meadow/wispy-grass-meadow_normal-ogl.png");
	m_triOcclusionTexs[2] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/wispy-grass-meadow/wispy-grass-meadow_arm.png");
	m_triEmissiveTexs[2] = nullptr;

	Reset();
}

GameRayMarching::~GameRayMarching()
{
	delete m_spectator;
	m_spectator = nullptr;

	DestroyRayMarchingConstants();
	DestroyShapeBuffer();
	DestroyDstTexture();
	DestroyDepthTexture();

	for (auto* shape : m_shapes)
	{
		delete shape;
	}
	m_shapes.clear();

}

void GameRayMarching::Update()
{
	UpdateDeveloperCheats();
	UpdatePerFrameConstants();
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

	float deltaSeconds = (float)m_clock->GetDeltaSeconds();

	UpdateShapes(deltaSeconds);

	UpdateRayMarching();
}

void GameRayMarching::Render() const
{

	g_theRenderer->BeginCamera(m_spectator->m_camera);
	// World-space drawing
	if (m_comboInt == 0)
	{
		RenderRayMarching(); // Notes: no depth writing
	}
	else if (m_comboInt == 1)
	{
		RenderMeshes();
	}
		
	g_theRenderer->EndCamera(m_spectator->m_camera);
	DebugRenderWorld(m_spectator->m_camera);

	//g_theRenderer->BeginCamera(m_screenCamera);
	//// Render UI
	//g_theRenderer->EndCamera(m_screenCamera);
	//DebugRenderScreen(m_screenCamera);
}

void GameRayMarching::Reset()
{

	for (auto* shape : m_shapes)
	{
		delete shape;
	}
	m_shapes.clear();

	for (int i = 0; i < INITIAL_SPHERE_COUNT; ++i)
	{
		SpawnSphere();
	}
}

void GameRayMarching::OnWindowResized()
{
	m_spectator->RefreshAspectRatio();
}

void GameRayMarching::UpdateShapes(float deltaSeconds)
{

	for (auto shape : m_shapes)
	{
		shape->m_position += deltaSeconds * shape->m_velocity;

		// Bounce with Walls
		if (shape->m_position.x > ACTIVITY_BOX_RADIUS)
		{
			shape->m_position.x = ACTIVITY_BOX_RADIUS;

			if (shape->m_velocity.x > 0.f)
			{
				shape->m_velocity.x *= -1.f;
			}
		}

		if (shape->m_position.x < -ACTIVITY_BOX_RADIUS)
		{
			shape->m_position.x = -ACTIVITY_BOX_RADIUS;

			if (shape->m_velocity.x < 0.f)
			{
				shape->m_velocity.x *= -1.f;
			}
		}

		if (shape->m_position.y > ACTIVITY_BOX_RADIUS)
		{
			shape->m_position.y = ACTIVITY_BOX_RADIUS;

			if (shape->m_velocity.y > 0.f)
			{
				shape->m_velocity.y *= -1.f;
			}
		}

		if (shape->m_position.y < -ACTIVITY_BOX_RADIUS)
		{
			shape->m_position.y = -ACTIVITY_BOX_RADIUS;

			if (shape->m_velocity.y < 0.f)
			{
				shape->m_velocity.y *= -1.f;
			}
		}

		if (shape->m_position.z > ACTIVITY_BOX_RADIUS)
		{
			shape->m_position.z = ACTIVITY_BOX_RADIUS;

			if (shape->m_velocity.z > 0.f)
			{
				shape->m_velocity.z *= -1.f;
			}
		}

		if (shape->m_position.z < -ACTIVITY_BOX_RADIUS)
		{
			shape->m_position.z = -ACTIVITY_BOX_RADIUS;

			if (shape->m_velocity.z < 0.f)
			{
				shape->m_velocity.z *= -1.f;
			}
		}
	}
}

void GameRayMarching::RenderMeshes() const
{
	std::vector<Vertex_PCUTBN> diffuseVerts;
	std::vector<unsigned int> diffuseIndices;

	for (GRMO_Sphere const* shape : m_shapes)
	{
		AddVertsForSphere3D(diffuseVerts, diffuseIndices, shape->m_position, shape->m_radius);
	}

	DiffuseRenderResources resources;
	resources.diffuseTextureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(nullptr);
	resources.diffuseSamplerIndex = g_theRenderer->GetDefaultSamplerIndex(SamplerMode::POINT_WARP);
	resources.cameraConstantsIndex = g_theRenderer->GetCurrentCameraConstantsIndex();
	resources.modelConstantsIndex = g_theRenderer->GetCurrentModelConstantsIndex();
	resources.lightConstantsIndex = g_theRenderer->GetCurrentLightConstantsIndex();

	g_theRenderer->SetGraphicsBindlessResources(sizeof(DiffuseRenderResources), &resources);

	g_theRenderer->BindShader(m_diffuseShader);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetRenderTargetFormats();

	g_theRenderer->DrawIndexedVertexArray(diffuseVerts, diffuseIndices);
}

void GameRayMarching::RenderFullScreenQuad() const
{
	FullScreenQuadResources resources;
	resources.textureIndex = g_theRenderer->GetSrvIndexFromLoadedTexture(nullptr, DefaultTexture::CheckerboardMagentaBlack2D);
	resources.samplerIndex = g_theRenderer->GetDefaultSamplerIndex(SamplerMode::BILINEAR_CLAMP);

	g_theRenderer->SetGraphicsBindlessResources(sizeof(FullScreenQuadResources), &resources);

	g_theRenderer->BindShader(m_fullScreenQuadShader);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRenderTargetFormats();

	g_theRenderer->DrawProcedural(6);

}

void GameRayMarching::UpdateRayMarching()
{
	int numOfShapes = (int)m_shapes.size();

	if (m_shapeBuffer == nullptr || m_shapeBuffer->GetSize() < numOfShapes * sizeof(SdfShape))
	{
		ResizeShapeBuffer(numOfShapes);
	}

	IntVec2 desiredDimensions = Window::s_mainWindow->GetClientDimensions();
	if (m_rayMarchingDstTexture == nullptr || m_rayMarchingDstTexture->GetDimensions() != desiredDimensions)
	{
		ResizeDstTexture(desiredDimensions);
	}

	if (m_rayMarchingDepthTexture == nullptr || m_rayMarchingDepthTexture->GetDimensions() != desiredDimensions)
	{
		ResizeDepthTexture(desiredDimensions);
	}

	// Get Data
	std::vector<SdfShape> shapeData;
	shapeData.reserve(numOfShapes);
	for (int i = 0; i < numOfShapes; ++i)
	{
		shapeData.push_back(m_shapes[i]->GetShape());
	}

	g_theRenderer->UpdateBuffer(*m_shapeBuffer, shapeData.size() * sizeof(SdfShape), shapeData.data());
	
	m_currentRayMarchingConstants.numOfShapes = numOfShapes;
	m_currentRayMarchingConstants.screenWidth = desiredDimensions.x;
	m_currentRayMarchingConstants.screenHeight = desiredDimensions.y;

	g_theRenderer->UpdateBuffer(*m_rayMarchingConstantBuffer, sizeof(SdfRayMarchingConstants), &m_currentRayMarchingConstants);
}

void GameRayMarching::RenderRayMarching() const
{
	g_theRenderer->TransitionToUnorderedAccess(*m_rayMarchingDstTexture);
	g_theRenderer->TransitionToUnorderedAccess(*m_rayMarchingDepthTexture);
	g_theRenderer->TransitionToGenericRead(*m_rayMarchingConstantBuffer);
	g_theRenderer->TransitionToGenericRead(*m_shapeBuffer);


	SdfRayMarchingResources rayMarchingRes;
	rayMarchingRes.engineConstantsIndex = g_theRenderer->GetCurrentEngineConstantsIndex();
	rayMarchingRes.cameraConstantsIndex = g_theRenderer->GetCurrentCameraConstantsIndex();
	rayMarchingRes.modelConstantsIndex = g_theRenderer->GetCurrentModelConstantsIndex();
	rayMarchingRes.lightConstantsIndex = g_theRenderer->GetCurrentLightConstantsIndex();
	rayMarchingRes.perFrameConstantsIndex = g_theRenderer->GetCurrentPerFrameConstantsIndex();

	rayMarchingRes.inputSdfShapesIndex = m_shapeBufferSRV.m_index;
	rayMarchingRes.outputTextureIndex = m_rayMarchingUAV.m_index;
	rayMarchingRes.outputDepthIndex = m_rayMarchingDepthUAV.m_index;
	rayMarchingRes.rayMarchingConstantsIndex = m_rayMarchingConstantBufferCBV.m_index;

	g_theRenderer->SetComputeBindlessResources(sizeof(SdfRayMarchingResources), &rayMarchingRes);

	g_theRenderer->BindComputeShader(m_rayMarchingShader);
	g_theRenderer->Dispatch2D(m_rayMarchingDstTexture->GetWidth(), m_rayMarchingDstTexture->GetWidth(), 8, 8); // need to be same in HLSL, may be larger


	//-----------------------------------------------------------------------------------------------
	// Draw full screen quad with depth
	g_theRenderer->TransitionToPixelShaderResource(*m_rayMarchingDstTexture);
	g_theRenderer->TransitionToPixelShaderResource(*m_rayMarchingDepthTexture);

	FullScreenQuadWithDepthResources fullScreenQuadWithDepthRes;
	fullScreenQuadWithDepthRes.textureIndex = m_rayMarchingSRV.m_index;
	fullScreenQuadWithDepthRes.depthTexIndex = m_rayMarchingDepthSRV.m_index;
	fullScreenQuadWithDepthRes.samplerIndex = g_theRenderer->GetDefaultSamplerIndex(SamplerMode::BILINEAR_CLAMP);

	g_theRenderer->SetGraphicsBindlessResources(sizeof(FullScreenQuadWithDepthResources), &fullScreenQuadWithDepthRes);

	g_theRenderer->BindShader(m_fullScreenQuadWithDepthShader);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetRenderTargetFormats();

	g_theRenderer->DrawProcedural(6);


}

void GameRayMarching::ResizeShapeBuffer(int numOfShapes)
{
	DestroyShapeBuffer();
	// If the buffer is nullptr or the size is not enough, create a new one
	BufferInit initData;
	initData.m_size = numOfShapes * sizeof(SdfShape);
	m_shapeBuffer = g_theRenderer->CreateBuffer(initData);

	m_shapeBufferSRV = g_theRenderer->AllocateStructuredBufferSRV(*m_shapeBuffer, sizeof(SdfShape), numOfShapes);
}

void GameRayMarching::DestroyShapeBuffer()
{
	g_theRenderer->DestroyBuffer(m_shapeBuffer);
	g_theRenderer->EnqueueDeferredRelease(m_shapeBufferSRV);
}

void GameRayMarching::ResizeDstTexture(IntVec2 dimensions)
{
	DestroyDstTexture();

	// If the texture is nullptr or the dimensions are not match, create a new one
	TextureInit initData;
	initData.m_width = dimensions.x;
	initData.m_height = dimensions.y;
	initData.m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	initData.m_allowUAV = true;
	
	m_rayMarchingDstTexture = g_theRenderer->CreateTexture(initData);

	m_rayMarchingUAV = g_theRenderer->AllocateUAV(*m_rayMarchingDstTexture);
	m_rayMarchingSRV = g_theRenderer->AllocateSRV(*m_rayMarchingDstTexture);
}

void GameRayMarching::DestroyDstTexture()
{
	g_theRenderer->DestroyTexture(m_rayMarchingDstTexture);
	g_theRenderer->EnqueueDeferredRelease(m_rayMarchingUAV);
	g_theRenderer->EnqueueDeferredRelease(m_rayMarchingSRV);
}

void GameRayMarching::ResizeDepthTexture(IntVec2 dimensions)
{
	DestroyDepthTexture();

	// If the texture is nullptr or the dimensions are not match, create a new one
	TextureInit initData;
	initData.m_width = dimensions.x;
	initData.m_height = dimensions.y;
	initData.m_format = DXGI_FORMAT_R32_FLOAT;
	initData.m_allowUAV = true;

	m_rayMarchingDepthTexture = g_theRenderer->CreateTexture(initData);

	m_rayMarchingDepthUAV = g_theRenderer->AllocateUAV(*m_rayMarchingDepthTexture);
	m_rayMarchingDepthSRV = g_theRenderer->AllocateSRV(*m_rayMarchingDepthTexture);
}

void GameRayMarching::DestroyDepthTexture()
{
	g_theRenderer->DestroyTexture(m_rayMarchingDepthTexture);
	g_theRenderer->EnqueueDeferredRelease(m_rayMarchingDepthUAV);
	g_theRenderer->EnqueueDeferredRelease(m_rayMarchingDepthSRV);
}

void GameRayMarching::CreateRayMarchingConstants()
{

	BufferInit initData;
	initData.m_isConstantBuffer = true;
	initData.m_size = sizeof(SdfRayMarchingConstants);

	m_rayMarchingConstantBuffer = g_theRenderer->CreateBuffer(initData);

	m_rayMarchingConstantBufferCBV = g_theRenderer->AllocateConstantBufferView(*m_rayMarchingConstantBuffer);
}

void GameRayMarching::DestroyRayMarchingConstants()
{
	g_theRenderer->DestroyBuffer(m_rayMarchingConstantBuffer);
	g_theRenderer->EnqueueDeferredRelease(m_rayMarchingConstantBufferCBV);
}

void GameRayMarching::ShowGameModeImGuiWindow()
{
	if (ImGui::Begin("SDF Ray Marching"))
	{
		if (ImGui::Button("Reset Scene")) 
		{
			Reset();
		}
		if (ImGui::Button("Add a sphere"))
		{
			SpawnSphere();
		}
		ImGui::DragFloat("Tolerance", &m_currentRayMarchingConstants.toleranceK, 0.01f, 0.1f, 2.f);

		const char* items[] = { "Ray Marching Mode", "Mesh Mode" };

		ImGui::Combo("combo", &m_comboInt, items, IM_ARRAYSIZE(items));

		ImGui::SliderFloat("UV Scale", &m_currentRayMarchingConstants.triplanarUVScale, 0.1f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("Blend Sharpness", &m_currentRayMarchingConstants.triplanarBlendSharpness, 0.1f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
	}

	ImGui::End();


}

void GameRayMarching::SpawnSphere()
{
	RandomNumberGenerator rng;

	GRMO_Sphere* newSphere = new GRMO_Sphere();

	Vec3 velocity = Vec3(rng.RollRandomFloatInRange(MIN_OBJECT_SPEED, MAX_OBJECT_SPEED),
		rng.RollRandomFloatInRange(MIN_OBJECT_SPEED, MAX_OBJECT_SPEED),
		rng.RollRandomFloatInRange(MIN_OBJECT_SPEED, MAX_OBJECT_SPEED));
	Vec3 signVelocity = Vec3(rng.RollRandomWithProbability(0.5f) ? 1.f : -1.f,
		rng.RollRandomWithProbability(0.5f) ? 1.f : -1.f,
		rng.RollRandomWithProbability(0.5f) ? 1.f : -1.f);
	newSphere->m_velocity = velocity * signVelocity;

	newSphere->m_radius = rng.RollRandomFloatInRange(MIN_SPHERE_RADIUS, MAX_SPHERE_RADIUS);

	const int matID = rng.RollRandomIntInRange(0, NUM_TRIPLANAR_TEX - 1);

	newSphere->m_triAlbedoTexID = g_theRenderer->GetSrvIndexFromLoadedTexture(m_triAlbedoTexs[matID], DefaultTexture::CheckerboardMagentaBlack2D);
	newSphere->m_triMRTexID = g_theRenderer->GetSrvIndexFromLoadedTexture(m_triMRTexs[matID], DefaultTexture::DefaultOcclusionRoughnessMetalnessMap);
	newSphere->m_triNormalTexID = g_theRenderer->GetSrvIndexFromLoadedTexture(m_triNormalTexs[matID], DefaultTexture::DefaultNormalMap);
	newSphere->m_triOcclusionTexID = g_theRenderer->GetSrvIndexFromLoadedTexture(m_triOcclusionTexs[matID], DefaultTexture::DefaultOcclusionRoughnessMetalnessMap);
	newSphere->m_triEmissiveTexID = g_theRenderer->GetSrvIndexFromLoadedTexture(m_triEmissiveTexs[matID], DefaultTexture::BlackOpaque2D);

	newSphere->m_color = Rgba8::MakeFromZeroToOne(rng.RollRandomFloatZeroToOne());

	m_shapes.push_back(newSphere);
}

