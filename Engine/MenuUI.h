#pragma once

#include <windows.h>
class URenderer;

enum class Screen { MainMenu, Running, EndingMenu, VictoryMenu, Count };
static Screen ScreenState = Screen::MainMenu;

struct MenuActions { bool menu = false; bool start = false; bool running = false; bool gameover;  bool exit = false; };

class MenuUI
{
public:
	MenuActions DrawMainMenu(URenderer& renderer, HWND hWnd);
	MenuActions DrawEndingMenu(URenderer& renderer, HWND hWnd);
	MenuActions DrawRunningMenu(URenderer& renderer, HWND hWnd); 
	MenuActions DrawVictoryMenu(URenderer& renderer, HWND hWnd); 
};
 