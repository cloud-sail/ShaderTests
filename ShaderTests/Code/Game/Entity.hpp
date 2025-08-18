#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <vector>


//-----------------------------------------------------------------------------------------------
class Texture;

//-----------------------------------------------------------------------------------------------
typedef std::vector<Entity*> EntityList;

//-----------------------------------------------------------------------------------------------
class Entity
{
public:
	Entity(Game* owner);
	virtual ~Entity() = default;

	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
	virtual Mat44 GetModelToWorldTransform() const;

public:
	Game* m_game = nullptr;

	Vec3 m_position;
	Vec3 m_scale = Vec3(1.f, 1.f, 1.f);
	EulerAngles m_orientation;

	Vec3 m_velocity;
	EulerAngles m_angularVelocity;

	Rgba8 m_color = Rgba8::OPAQUE_WHITE;
};

