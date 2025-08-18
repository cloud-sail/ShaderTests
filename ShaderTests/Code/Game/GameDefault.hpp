#pragma once
#include "Game/Game.hpp"


class GameDefault : public Game
{
public:
	GameDefault();
	~GameDefault();
	void Update() override;
	void Render() const override;
	void Reset() override;
	void OnWindowResized() override;

private:
	SpectatorCamera* m_spectator = nullptr;
};

