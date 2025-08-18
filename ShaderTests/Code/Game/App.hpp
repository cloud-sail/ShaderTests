#pragma once
#include "Game/GameCommon.hpp"
//-----------------------------------------------------------------------------------------------
class Game;

//-----------------------------------------------------------------------------------------------
class App
{
public:
    App();
    ~App();
    void Startup();
    void Shutdown();
    void RunMainLoop();
    void RunFrame();

    void HandleQuitRequested();
    void HandleWindowResized();
    bool IsQuitting() const { return m_isQuitting; }

private:
    void BeginFrame();
    void Update();
    void Render() const;
    void EndFrame();
    
    void LoadGameConfig(char const* gameConfigXmlFilePath);

    Game* CreateNewGameForMode(GameMode mode);
	void SwitchToPreviousMode();
	void SwitchToNextMode();


    // #ToDo
    // Back to default test Button
    // default test imgui panel have scene selection UI

private:
	GameMode m_currentGameMode = GAME_MODE_DEFAULT;
	Game* m_theGame = nullptr;

    bool m_isQuitting = false;
};
