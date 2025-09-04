#pragma once

#include <windows.h>
class URenderer;

enum class Screen { MainMenu, Running, EndingMenu, VictoryMenu, Count };
static Screen ScreenState = Screen::MainMenu;

struct MenuActions { bool menu = false; bool start = false; bool running = false; bool gameover = false;  bool exit = false; };

class MenuUI
{
public:
	MenuActions DrawMainMenu(URenderer& renderer, HWND hWnd);
	MenuActions DrawEndingMenu(URenderer& renderer, HWND hWnd);
	MenuActions DrawRunningMenu(URenderer& renderer, HWND hWnd); 
	MenuActions DrawVictoryMenu(URenderer& renderer, HWND hWnd); 

public:
	bool SettingReactUI(URenderer renderer, class ID3D11ShaderResourceView* SRV, float winSize[2], float mousePos[2],
		float targetSize[2], float ratio[2], float posOffset[2]);

};
 