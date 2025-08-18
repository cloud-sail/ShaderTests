#include "Game/GameDefault.hpp"
#include "Game/SpectatorCamera.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"

GameDefault::GameDefault()
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

}

GameDefault::~GameDefault()
{
	delete m_spectator;
	m_spectator = nullptr;
}

void GameDefault::Update()
{
	UpdateDeveloperCheats();
	ShowCommonImGuiWindow();
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

void GameDefault::Render() const
{

	g_theRenderer->BeginCamera(m_spectator->m_camera);
	// World-space drawing
	g_theRenderer->EndCamera(m_spectator->m_camera);
	DebugRenderWorld(m_spectator->m_camera);

	//g_theRenderer->BeginCamera(m_screenCamera);
	//// Render UI
	//g_theRenderer->EndCamera(m_screenCamera);
	//DebugRenderScreen(m_screenCamera);
}

void GameDefault::Reset()
{
}

void GameDefault::OnWindowResized()
{
	m_spectator->RefreshAspectRatio();
}
