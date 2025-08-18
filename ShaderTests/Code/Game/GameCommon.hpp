#pragma once
#include "Engine/Core/EngineCommon.hpp"

//-----------------------------------------------------------------------------------------------
class AudioSystem;
class InputSystem;
class Renderer;
class Window;
class App;
class Game;

//-----------------------------------------------------------------------------------------------
extern AudioSystem*		g_theAudio;
extern InputSystem*		g_theInput;
extern Renderer*		g_theRenderer;
extern Window*			g_theWindow;
extern App*				g_theApp;

//-----------------------------------------------------------------------------------------------
extern bool g_isDebugDraw;


//-----------------------------------------------------------------------------------------------
class Clock;

class SpectatorCamera;
class Entity;

//-----------------------------------------------------------------------------------------------
// Gameplay Constants
constexpr float SCREEN_SIZE_X = 1600.f;
constexpr float SCREEN_SIZE_Y = 800.f;

constexpr float CAMERA_MOVE_SPEED = 2.f;
constexpr float CAMERA_YAW_TURN_RATE = 60.f;
constexpr float CAMERA_PITCH_TURN_RATE = 60.f;
constexpr float CAMERA_ROLL_TURN_RATE = 90.f;
constexpr float CAMERA_SPEED_FACTOR = 10.f;

constexpr float CAMERA_MAX_PITCH = 85.f;
constexpr float CAMERA_MAX_ROLL = 45.f;


//-----------------------------------------------------------------------------------------------
enum GameMode
{
	GAME_MODE_DEFAULT,
	GAME_MODE_RAY_MARCHING,
	GAME_MODE_TRIPLANAR_MAPPING,
	GAME_MODE_PBR,
	GAME_MODE_NUM
};


struct FullScreenQuadResources
{
	uint32_t textureIndex = 0;
	uint32_t samplerIndex = 0;
};

struct FullScreenQuadWithDepthResources
{
	uint32_t textureIndex = 0;
	uint32_t depthTexIndex = 0;
	uint32_t samplerIndex = 0;
};

struct TriplanarRenderResources
{
	uint32_t cameraConstantsIndex = 0;
	uint32_t modelConstantsIndex = 0;
	uint32_t lightConstantsIndex = 0;

	float uvScale = 1.f;
	float blendSharpness = 1.f;

	uint32_t diffuseTextureIndex = 0;
	uint32_t diffuseSamplerIndex = 0;
};


struct TriplanarPBRRenderResources
{
	uint32_t engineConstantsIndex = 0;
	uint32_t cameraConstantsIndex = 0;
	uint32_t modelConstantsIndex = 0;
	uint32_t lightConstantsIndex = 0;

	float uvScale = 1.f;
	float blendSharpness = 1.f;

	uint32_t albedoTextureIndex = 0;
	uint32_t metallicRoughnessTextureIndex = 0;
	uint32_t normalTextureIndex = 0;
	uint32_t occlusionTextureIndex = 0;
	uint32_t emissiveTextureIndex = 0;

	uint32_t samplerIndex = 0;
};

