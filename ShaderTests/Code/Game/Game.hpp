#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/RendererCommon.hpp"
#include "Engine/Input/InputSystem.hpp"

// ----------------------------------------------------------------------------------------------
class Game
{
public:
	Game();
	virtual ~Game();
	virtual void Update() = 0;
	virtual void Render() const = 0;;
	virtual void Reset() = 0; // F8

	virtual void OnWindowResized() = 0; // Event WINDOW_RESIZE_EVENT, refresh the setting of the camera

	void UpdateDeveloperCheats();
	void UpdatePerFrameConstants();
	CursorMode GetCursorMode() const { return m_cursorMode; }
protected:
	Clock* m_clock = nullptr;
	CursorMode m_cursorMode = CursorMode::POINTER;

// Common ImGui
protected:
	void ShowCommonImGuiWindow();
	void ToggleCursorMode();
	void DebugDrawLights();

private:
	void ShowLightControlWindow(bool* pOpen);

	bool m_showLightControlWindow = true;
	LightConstants m_lightConstants;
	void ResetLighting();
	void ApplyLighting();
	float m_lightDirBuffer[MAX_LIGHTS][3]; // for ImGui

	// Engine Constants
	int m_debugInt = 0;
	float m_debugFloat = 0.f;
};
