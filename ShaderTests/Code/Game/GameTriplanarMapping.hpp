#pragma once
#include "Game/Game.hpp"

class IndexBuffer;
class VertexBuffer;

class GameTriplanarMapping : public Game
{
public:
	GameTriplanarMapping();
	~GameTriplanarMapping();
	void Update() override;
	void Render() const override;
	void Reset() override;
	void OnWindowResized() override;

private:
	void LoadModel();
	void RenderDiffuseModel() const;
	void RenderPBRModel() const;

private:
	SpectatorCamera* m_spectator = nullptr;

	// Render one object
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;

	Texture* m_diffuseTexture = nullptr;
	SamplerMode m_diffuseSampler = SamplerMode::BILINEAR_WRAP;
	Shader* m_shader = nullptr;

	Texture* m_albedoTexture = nullptr;
	Texture* m_metallicRoughnessTexture = nullptr;
	Texture* m_normalTexture = nullptr;
	Texture* m_occlusionTexture = nullptr;
	Texture* m_emissiveTexture = nullptr;
	SamplerMode m_sampler = SamplerMode::BILINEAR_WRAP;
	Shader* m_pbrShader = nullptr;

	bool m_isDiffuseMode = false;

private:
	void ShowGameModeImGuiWindow();

private:
	float m_uvScale = 1.f;
	float m_blendSharpness = 1.f;
};

