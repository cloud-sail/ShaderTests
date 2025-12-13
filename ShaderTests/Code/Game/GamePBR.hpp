#pragma once
#include "Game/Game.hpp"

class IndexBuffer;
class VertexBuffer;


struct TestPBRRenderResources
{
	uint32_t albedoTextureIndex = INVALID_INDEX_U32;
	uint32_t metallicRoughnessTextureIndex = INVALID_INDEX_U32;
	uint32_t normalTextureIndex = INVALID_INDEX_U32;
	uint32_t occlusionTextureIndex = INVALID_INDEX_U32;
	uint32_t emissiveTextureIndex = INVALID_INDEX_U32;

	uint32_t samplerIndex = INVALID_INDEX_U32;

	uint32_t engineConstantsIndex = INVALID_INDEX_U32;
	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
	uint32_t lightConstantsIndex = INVALID_INDEX_U32;

	uint32_t radianceTextureIndex = INVALID_INDEX_U32;
	uint32_t irradianceTextureIndex = INVALID_INDEX_U32;
	uint32_t brdfLutTextureIndex = INVALID_INDEX_U32;

	float debugMetallic = 1.f;
	float debugRoughness = 0.f;
	float uvScale = 0.25f;
};


class GamePBR : public Game
{
public:
	GamePBR();
	~GamePBR();
	void Update() override;
	void Render() const override;
	void Reset() override;
	void OnWindowResized() override;

private:
	void LoadIBL();
	void LoadModel();
	void RenderModel() const;

private:
	SpectatorCamera* m_spectator = nullptr;

	// Render one object
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;

	Texture* m_albedoTexture = nullptr;
	Texture* m_metallicRoughnessTexture = nullptr;
	Texture* m_normalTexture = nullptr;
	Texture* m_occlusionTexture = nullptr;
	Texture* m_emissiveTexture = nullptr;
	SamplerMode m_sampler = SamplerMode::BILINEAR_WRAP;

	Shader* m_shader = nullptr;


private:
	void ShowGameModeImGuiWindow();

private:
	float m_debugMetallic = 1.f;
	float m_debugRoughness = 0.f;

private:
	// #ToDo A new engine class
	void RenderSkybox() const;

private:
	// IBL and Skybox
	Shader* m_skyboxShader = nullptr;
	Texture* m_skyboxTexture = nullptr;
	Texture* m_radianceTexture = nullptr;
	Texture* m_irradianceTexture = nullptr;
	Texture* m_brdfLutTexture = nullptr;
};


