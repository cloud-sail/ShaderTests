#pragma once
#include "Game/Game.hpp"
#include "Engine/Core/Vertex_PNMD.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"


struct GMaterial
{
	uint32_t albedoTextureIndex = INVALID_INDEX_U32;
	uint32_t metallicRoughnessTextureIndex = INVALID_INDEX_U32;
	uint32_t normalTextureIndex = INVALID_INDEX_U32;
	uint32_t occlusionTextureIndex = INVALID_INDEX_U32;
	uint32_t emissiveTextureIndex = INVALID_INDEX_U32;

	uint32_t samplerIndex = INVALID_INDEX_U32;
};

struct TriplanarMaterialBlendResources
{
	uint32_t materialBufferIndex = INVALID_INDEX_U32; // StructuredBuffer<Material>

	float uvScale = 1.f;
	float blendSharpness = 1.f;

	uint32_t engineConstantsIndex = INVALID_INDEX_U32;
	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
	uint32_t lightConstantsIndex = INVALID_INDEX_U32;
	uint32_t perFrameConstantsIndex = INVALID_INDEX_U32;
};



class GameTriplanarMaterialBlend : public Game
{
public:
	GameTriplanarMaterialBlend();
	~GameTriplanarMaterialBlend();
	void Update() override;
	void Render() const override;
	void Reset() override;
	void OnWindowResized() override;

private:
	void Initialize();
	void UpdateModel();
	void RenderModel() const;

private:
	SpectatorCamera* m_spectator = nullptr;

	std::vector<Vertex_PNMD> m_vertices;
	std::vector<unsigned int> m_indices;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;


	Buffer* m_materialBuffer = nullptr; // Structured Buffer
	DescriptorHandle m_materialBufferSRV;

	Shader* m_shader = nullptr;

private:
	void ShowGameModeImGuiWindow();

private:
	float m_uvScale = 1.f;
	float m_blendSharpness = 1.f;

private:
	// Billboard Quad

	Vec3 m_position;
	EulerAngles m_orientation;
};

