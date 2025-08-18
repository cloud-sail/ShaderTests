#pragma once
#include "Game/Entity.hpp"
#include "Engine/Renderer/Camera.hpp"

class Player : public Entity
{
public:
	Player(Game* owner);
	virtual ~Player() = default;

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

protected:
	void UpdateOrientation(float deltaSeconds);
	void UpdatePosition(float deltaSeconds);
	void UpdateCamera();

public:
	Camera m_camera;
};

