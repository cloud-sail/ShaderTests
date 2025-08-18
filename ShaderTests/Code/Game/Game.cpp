#include "Game/Game.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Window/Window.hpp"
#include "ThirdParty/imgui/imgui.h"

Game::Game()
{
	m_clock = new Clock();
	ResetLighting();
}

Game::~Game()
{
	delete m_clock;
	m_clock = nullptr;

	DebugRenderClear();
}

void Game::UpdateDeveloperCheats()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		g_isDebugDraw = !g_isDebugDraw;
	}

	m_clock->SetTimeScale(g_theInput->IsKeyDown(KEYCODE_T) ? 0.1 : 1.0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_P))
	{
		m_clock->TogglePause();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_O))
	{
		m_clock->StepSingleFrame();
	}
}

void Game::UpdatePerFrameConstants()
{
	// Update PerFrameData
	PerFrameConstants perFrameCB;
	IntVec2 clientDimensions = Window::s_mainWindow->GetClientDimensions();
	perFrameCB.m_resolution = Vec3((float)clientDimensions.x, (float)clientDimensions.y, 1.f);

	perFrameCB.m_timeSeconds = (float)m_clock->GetTotalSeconds();
	g_theRenderer->SetPerFrameConstants(perFrameCB);
}

void Game::ShowCommonImGuiWindow()
{
	if (ImGui::Begin("Control Panel"))
	{
		ImGui::Text("Press F to free the cursor");
		ImGui::Checkbox("Light Control Window", &m_showLightControlWindow);
		ImGui::InputInt("debug int", &m_debugInt, 1);
		ImGui::DragFloat("debug float", &m_debugFloat);
		ImGui::Text("F1 - Toggle Debug Draw\nF6 - previous scene\nF7 - next scene\nF8 - reset\nT - Slow motion\nP - Toggle Pause\nO - Step Single Frame");
	}


	if (m_showLightControlWindow)
	{
		ShowLightControlWindow(&m_showLightControlWindow);
	}

	ImGui::End();
	g_theRenderer->SetEngineConstants(m_debugInt, m_debugFloat);
}

void Game::ToggleCursorMode()
{
	if (m_cursorMode == CursorMode::FPS)
	{
		m_cursorMode = CursorMode::POINTER;
	}
	else
	{
		m_cursorMode = CursorMode::FPS;
	}
}

void Game::DebugDrawLights()
{
	for (int i = 0; i < m_lightConstants.m_numLights; ++i)
	{
		Light const& light = m_lightConstants.m_lights[i];

		Rgba8 color = Rgba8(DenormalizeByte(light.m_color[0]), DenormalizeByte(light.m_color[1]), DenormalizeByte(light.m_color[2]));
		DebugAddWorldSphere(light.m_worldPosition, 0.1f, 0.f, color);

		if (light.m_outerDotThreshold > -0.99f)
		{
			DebugAddWorldWirePenumbraNoneCull(light.m_worldPosition, light.m_spotForwardNormal, light.m_outerRadius, light.m_outerDotThreshold, 0.f, color);
		}

		if (light.m_innerDotThreshold > -0.99f)
		{
			DebugAddWorldWirePenumbraNoneCull(light.m_worldPosition, light.m_spotForwardNormal, light.m_innerRadius, light.m_innerDotThreshold, 0.f, color);
		}
		else
		{
			DebugAddWorldWireSphereNoneCull(light.m_worldPosition, light.m_innerRadius, 0.f, color);
			DebugAddWorldWireSphereNoneCull(light.m_worldPosition, light.m_outerRadius, 0.f, color);
		}
	}
}

void Game::ShowLightControlWindow(bool* pOpen)
{
	if (ImGui::Begin("Light Control Panel", pOpen))
	{
		if (ImGui::Button("Reset Lighting")) 
		{
			ResetLighting();
		}
		//-----------------------------------------------------------------------------------------------
		ImGui::SeparatorText("Sun");
		ImGui::ColorEdit4("Sun Color", m_lightConstants.m_sunColor);
		static float sunDir[3] = { 1.f, 2.f, -1.f };
		if (ImGui::DragFloat3("Sun Direction", sunDir, 0.02f, -10.f, 10.f, "%.2f"))
		{
			Vec3 newSunDir = Vec3(sunDir[0], sunDir[1], sunDir[2]);
			m_lightConstants.m_sunNormal = newSunDir.GetNormalized();
		}

		//-----------------------------------------------------------------------------------------------
		ImGui::SeparatorText("Lights");
		ImGui::SliderInt("Number of Lights", &m_lightConstants.m_numLights, 0, MAX_LIGHTS);

		for (int i = 0; i < m_lightConstants.m_numLights; ++i)
		{
			ImGui::PushID(i);
			Light& light = m_lightConstants.m_lights[i];

			if (ImGui::CollapsingHeader(Stringf("Light %d", i).c_str()))
			{
				ImGui::ColorEdit4("Color", light.m_color);
				//ImGui::InputFloat3("World Position", (float*)&(light.m_worldPosition), "%.2f");
				ImGui::DragFloat3("World Position", (float*)&(light.m_worldPosition), 0.1f, -10.f, 18.f, "%.2f");


				if (ImGui::DragFloat3("Forward Normal", m_lightDirBuffer[i], 0.01f))
				{
					Vec3 newDir = Vec3(m_lightDirBuffer[i][0], m_lightDirBuffer[i][1], m_lightDirBuffer[i][2]);
					light.m_spotForwardNormal = newDir.GetNormalized();
				}

				ImGui::SliderFloat("Ambience", &light.m_ambience, 0.0f, 1.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
				ImGui::SliderFloat("Inner Radius", &light.m_innerRadius, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
				ImGui::SliderFloat("Outer Radius", &light.m_outerRadius, light.m_innerRadius + 0.001f, 12.0f);

				ImGui::SliderFloat("Inner Penumbra Dot", &light.m_innerDotThreshold, -1.0f, 1.0f);
				ImGui::SliderFloat("Outer Penumbra Dot", &light.m_outerDotThreshold, -1.0f, 1.0f);
			}

			ImGui::PopID();
		}

	}
	ImGui::End();

	ApplyLighting();
}

void Game::ResetLighting()
{
	m_lightConstants.m_sunColor[0] = 1.f;
	m_lightConstants.m_sunColor[1] = 1.f;
	m_lightConstants.m_sunColor[2] = 1.f;
	m_lightConstants.m_sunColor[3] = 0.7f;

	m_lightConstants.m_sunNormal = Vec3(1.f, 2.f, -1.f).GetNormalized();

	m_lightConstants.m_numLights = 3;

	// Point Light
	Light pointLight;
	pointLight.SetColor(0.f, 1.f, 1.f, 0.8f);
	pointLight.m_worldPosition = Vec3(4.f, 4.f, 2.f);
	pointLight.m_innerRadius = 4.f;
	pointLight.m_outerRadius = 6.f;

	m_lightConstants.m_lights[0] = pointLight;

	Light spotLight1;
	spotLight1.SetColor(1.f, 1.f, 0.f, 0.7f);
	spotLight1.m_worldPosition = Vec3(4.f, -1.62f, 4.516f);
	spotLight1.m_spotForwardNormal = Vec3(0.f, 1.f, -1.f).GetNormalized();
	spotLight1.m_innerRadius = 5.f;
	spotLight1.m_outerRadius = 10.f;
	spotLight1.m_innerDotThreshold = 0.9f;
	spotLight1.m_outerDotThreshold = 0.7f;

	m_lightConstants.m_lights[1] = spotLight1;

	Light spotLight2;
	spotLight2.SetColor(0.514f, 0.2706f, 1.f, 0.7f);
	spotLight2.m_worldPosition = Vec3(4.f, 9.57f, 4.516f);
	spotLight2.m_spotForwardNormal = Vec3(0.f, -1.f, -1.f).GetNormalized();
	spotLight2.m_innerRadius = 5.f;
	spotLight2.m_outerRadius = 10.f;
	spotLight2.m_innerDotThreshold = 0.9f;
	spotLight2.m_outerDotThreshold = 0.7f;

	m_lightConstants.m_lights[2] = spotLight2;

	for (int i = 0; i < m_lightConstants.m_numLights; ++i) {
		m_lightDirBuffer[i][0] = m_lightConstants.m_lights[i].m_spotForwardNormal.x;
		m_lightDirBuffer[i][1] = m_lightConstants.m_lights[i].m_spotForwardNormal.y;
		m_lightDirBuffer[i][2] = m_lightConstants.m_lights[i].m_spotForwardNormal.z;
	}

	ApplyLighting();
}

void Game::ApplyLighting()
{
	g_theRenderer->SetLightConstants(m_lightConstants);
}
