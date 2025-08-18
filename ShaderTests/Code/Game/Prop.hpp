#pragma once
#include "Game/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include <vector>


//-----------------------------------------------------------------------------------------------
class Prop : public Entity
{
public:
	Prop(Game* owner);
	virtual ~Prop() = default;

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

public:
	std::vector<Vertex_PCU> m_vertexes;
	Texture*				m_texture = nullptr;
};
