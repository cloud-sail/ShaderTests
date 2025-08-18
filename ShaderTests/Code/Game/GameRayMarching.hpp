#pragma once
#include "Game/Game.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Renderer/RendererCommon.hpp"

/*
Reference:
https://michaelwalczyk.com/blog-ray-marching.html
https://github.com/TheAllenChou/unity-ray-marching
https://iquilezles.org/articles/smin/
https://iquilezles.org/articles/normalsSDF/
*/



constexpr int NUM_TRIPLANAR_TEX = 3;


// 
// Notes: must be same as the struct in hlsl
struct SdfShape
{
	// Type: Sphere Only
	// Color: 
	// Bool Operation: Union Only
	// Orientation(Quaternion): Spheres do not need this

	enum
	{
		SDF_SPHERE = 0,
	};


	int m_type = 0;
	int m_padding0 = 0;
	int m_padding1 = 0;
	uint32_t m_triAlbedoTexID = INVALID_INDEX_U32;

	uint32_t m_triMRTexID = INVALID_INDEX_U32;
	uint32_t m_triNormalTexID = INVALID_INDEX_U32;
	uint32_t m_triOcclusionTexID = INVALID_INDEX_U32;
	uint32_t m_triEmissiveTexID = INVALID_INDEX_U32;

	float m_color[4];
	Vec4 m_data0; // center.xyz + radius

	static SdfShape MakeSphere(Vec3 center, float radius, Rgba8 color = Rgba8::OPAQUE_WHITE);

};


struct SdfRayMarchingResources
{
	uint32_t engineConstantsIndex = INVALID_INDEX_U32;
	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
	uint32_t lightConstantsIndex = INVALID_INDEX_U32;
	uint32_t perFrameConstantsIndex = INVALID_INDEX_U32;

	uint32_t inputSdfShapesIndex = INVALID_INDEX_U32; // StructuredBuffer<SdfShape>
	uint32_t outputTextureIndex = INVALID_INDEX_U32; // RWTexture2D<float4>
	uint32_t outputDepthIndex = INVALID_INDEX_U32; // RWTexture2D<float>
	uint32_t rayMarchingConstantsIndex = INVALID_INDEX_U32;
};

struct SdfRayMarchingConstants
{
	int maxSteps = 100;
	float minHitDistance = 0.001f;
	float maxTraceDistance = 1000.f;
	float toleranceK = 0.5f;

	int numOfShapes = 0;
	int screenWidth = 0;
	int screenHeight = 0;
	float padding;

	float triplanarUVScale = 1.f;
	float triplanarBlendSharpness = 1.f;
	float padding1;
	float padding2;
};


class GRMO_Sphere
{
public:
	SdfShape GetShape() const;

public:
	Vec3 m_position;
	Vec3 m_velocity; // wandering inside of box, flip velocity when going outside
	Rgba8 m_color = Rgba8::OPAQUE_WHITE;

	uint32_t m_triAlbedoTexID = INVALID_INDEX_U32;
	uint32_t m_triMRTexID = INVALID_INDEX_U32;
	uint32_t m_triNormalTexID = INVALID_INDEX_U32;
	uint32_t m_triOcclusionTexID = INVALID_INDEX_U32;
	uint32_t m_triEmissiveTexID = INVALID_INDEX_U32;

	float m_radius = 0.f;

};


class GameRayMarching : public Game
{
public:
	GameRayMarching();
	~GameRayMarching();
	void Update() override;
	void Render() const override;
	void Reset() override;
	void OnWindowResized() override;

private:
	SpectatorCamera* m_spectator = nullptr;

private:
	void UpdateShapes(float deltaSeconds);
	void RenderMeshes() const; // only for test
	void RenderFullScreenQuad() const; // only for test

	void UpdateRayMarching(); // try not to change the shape list after it
	void RenderRayMarching() const;

	void ResizeShapeBuffer(int numOfShapes);
	void DestroyShapeBuffer();

	void ResizeDstTexture(IntVec2 dimensions);
	void DestroyDstTexture();

	void ResizeDepthTexture(IntVec2 dimensions);
	void DestroyDepthTexture();

	void CreateRayMarchingConstants();
	void DestroyRayMarchingConstants();

private:
	void ShowGameModeImGuiWindow();
private:
	int m_comboInt = 0;

private:
	void SpawnSphere();

private:
	std::vector<GRMO_Sphere*> m_shapes;

	Buffer* m_shapeBuffer = nullptr; // Structured Buffer
	DescriptorHandle m_shapeBufferSRV;

	// Methods to use:
	// CreateBuffer
	// UpdateBuffer
	// DestroyBuffer
	// AllocateStructuredBufferSRV
	// TransitionToGenericRead
	// TransitionToCopyDest
	// EnqueueDeferredRelease(DescriptorHandle& handle)

	Texture* m_rayMarchingDstTexture = nullptr; // compute Shader will write on this texture. In the end, render a full screen quad
	DescriptorHandle m_rayMarchingUAV;
	DescriptorHandle m_rayMarchingSRV;

	Texture* m_rayMarchingDepthTexture = nullptr;
	DescriptorHandle m_rayMarchingDepthUAV;
	DescriptorHandle m_rayMarchingDepthSRV;

	// Update it every frame
	SdfRayMarchingConstants m_currentRayMarchingConstants;
	Buffer* m_rayMarchingConstantBuffer = nullptr;
	DescriptorHandle m_rayMarchingConstantBufferCBV;


	Shader* m_rayMarchingShader = nullptr;
	Shader* m_fullScreenQuadShader = nullptr;
	Shader* m_fullScreenQuadWithDepthShader = nullptr;
	Shader* m_diffuseShader = nullptr;


	Texture* m_triAlbedoTexs[NUM_TRIPLANAR_TEX] = {};
	Texture* m_triMRTexs[NUM_TRIPLANAR_TEX] = {};
	Texture* m_triNormalTexs[NUM_TRIPLANAR_TEX] = {};
	Texture* m_triOcclusionTexs[NUM_TRIPLANAR_TEX] = {};
	Texture* m_triEmissiveTexs[NUM_TRIPLANAR_TEX] = {};


};

