#include "MenuUI.h"
#include "URenderer.h"
#include "UIInfo.h"
#include "InputManager.h"
#include "SoundManager.h"


bool  MenuUI::SettingReactUI(URenderer renderer, ID3D11ShaderResourceView* SRV, float winSize[2], float mousePos[2],
	float targetSize[2], float ratio[2], float posOffset[2])
{ 
	float hoveringSize[2] = { targetSize[0] - posOffset[0], targetSize[1] - posOffset[1] };
	UIReact reactStart = MakeRect(winSize, hoveringSize, ratio);
	bool hoverTest = CheckMouseOnUI(reactStart, mousePos[0], mousePos[1]);
	renderer.UpdateUIConstant(winSize, targetSize, hoverTest, ratio);
	renderer.PrepareShaderUI(SRV); 

	return hoverTest;
}


MenuActions MenuUI::DrawMainMenu(URenderer& renderer, HWND hWnd)
{
	MenuActions MenuAction{};

	RECT rect;
	GetClientRect(hWnd, &rect);
	float winW = (float)(rect.right - rect.left);
	float winH = (float)(rect.bottom - rect.top);
	float winSize[2] = { winW, winH };

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	int mouseX = pt.x;
	int mouseY = pt.y;
	float mousePos[2] = { mouseX, mouseY };


	// Title UI   
	float titleRatio[2] = { 0.5f, 0.3f };
	float targetSize[2] = { 500, 500 };
	renderer.UpdateUIConstant(winSize, targetSize, true, titleRatio);
	renderer.PrepareShaderUI(renderer.UITitleSRV);

	// Start UI  
	float startRatio[2] = { 0.5f, 0.7f };
	float startUIOffset[2] = { 50.0f, 100.f };
	targetSize[0] = 200; targetSize[1] = 200;
	float hoveringSize[2] = { targetSize[0] - startUIOffset[0], targetSize[1] - startUIOffset[1] }; 
	bool startHoverTest = SettingReactUI(renderer, renderer.UIStartSRV, winSize, mousePos, targetSize, startRatio, startUIOffset);

 
	// Exit UI 
	float exitRatio[2] = { 0.5f, 0.8f };
	float exitUIOffset[2] = { 50.0f, 100.0f };
	targetSize[0] = 200; targetSize[1] = 200;
	hoveringSize[0] = targetSize[0] - exitUIOffset[0]; hoveringSize[1] = targetSize[1] - exitUIOffset[1];

	UIReact reactExit = MakeRect(winSize, hoveringSize, exitRatio);
	bool exitHoverTest = CheckMouseOnUI(reactExit, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, exitHoverTest, exitRatio);
	renderer.PrepareShaderUI(renderer.UIExitSRV);

	// --- Handle Button Click --- 
	if (InputManager::Input().IsClicked(MouseButton::Left) && startHoverTest)
	{
		MenuAction.start = true;
		USoundManager::UIClick();
	}
	if (InputManager::Input().IsClicked(MouseButton::Left) && exitHoverTest)
	{
		MenuAction.exit = true;
		USoundManager::UIClick(); 
	}

	return MenuAction;
}


MenuActions MenuUI::DrawEndingMenu(URenderer& renderer, HWND hWnd)
{
	MenuActions MenuAction;
	MenuAction.gameover = true;
	RECT rect;
	GetClientRect(hWnd, &rect);
	float winW = (float)(rect.right - rect.left);
	float winH = (float)(rect.bottom - rect.top);
	float winSize[2] = { winW, winH };
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	int mouseX = pt.x;
	int mouseY = pt.y;
	float mousePos[2] = { mouseX, mouseY };

	// Name UI
	float nameRatio[2] = { 0.5f, 0.5f };
	float targetSize[2] = { winW, winH };
	renderer.UpdateUIConstant(winSize, targetSize, false, nameRatio);
	renderer.PrepareShaderUI(renderer.UINameSRV);
	// Title UI
	float titleRatio[2] = { 0.5f, 0.3f };
	targetSize[0] = 500; targetSize[1] = 500;
	renderer.UpdateUIConstant(winSize, targetSize, true, titleRatio);
	renderer.PrepareShaderUI(renderer.UIGameOverSRV);
	// Start UI
	float startRatio[2] = { 0.5f, 0.7f };
	float startUIOffset[2] = { 50.0f, 100.f };
	targetSize[0] = 200; targetSize[1] = 200;
	float hoveringSize[2] = { targetSize[0] - startUIOffset[0], targetSize[1] - startUIOffset[1] };
	UIReact reactStart = MakeRect(winSize, hoveringSize, startRatio);
	bool startHoverTest = CheckMouseOnUI(reactStart, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, startHoverTest, startRatio);
	renderer.PrepareShaderUI(renderer.UIStartSRV);

	// Menu UI
	float menuRatio[2] = { 0.5f, 0.8f };
	float menuUIOffset[2] = { 50.0f, 100.0f };
	targetSize[0] = 200; targetSize[1] = 150;
	hoveringSize[0] = targetSize[0] - menuUIOffset[0]; hoveringSize[1] = targetSize[1] - menuUIOffset[1];
	UIReact reactMenu = MakeRect(winSize, hoveringSize, menuRatio);
	bool menuHoverTest = CheckMouseOnUI(reactMenu, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, menuHoverTest, menuRatio);
	renderer.PrepareShaderUI(renderer.UIMenuSRV);
	// Exit UI
	float exitRatio[2] = { 0.9f, 0.95f };
	float exitUIOffset[2] = { 50.0f, 100.0f };
	targetSize[0] = 200; targetSize[1] = 200;
	hoveringSize[0] = targetSize[0] - exitUIOffset[0]; hoveringSize[1] = targetSize[1] - exitUIOffset[1];
	UIReact reactExit = MakeRect(winSize, hoveringSize, exitRatio);
	bool exitHoverTest = CheckMouseOnUI(reactExit, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, exitHoverTest, exitRatio);
	renderer.PrepareShaderUI(renderer.UIExitSRV);
	MenuActions action; 

	//Click Event
	if (InputManager::Input().IsClicked(MouseButton::Left) && startHoverTest)
	{
		action.start = true;
		MenuAction = action;
	}
	if (InputManager::Input().IsClicked(MouseButton::Left) && exitHoverTest)
	{
		action.exit = true;
		MenuAction = action;
		//NewController->bIsEnabled = true;
	}
	if (InputManager::Input().IsClicked(MouseButton::Left) && menuHoverTest)
	{
		action.menu = true;
		MenuAction = action;
	}
	return MenuAction;
}

MenuActions MenuUI::DrawRunningMenu(URenderer& renderer, HWND hWnd)
{
	MenuActions MenuAction;
	MenuAction.gameover = true;
	RECT rect;
	GetClientRect(hWnd, &rect);
	float winW = (float)(rect.right - rect.left);
	float winH = (float)(rect.bottom - rect.top);
	float winSize[2] = { winW, winH };
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	int mouseX = pt.x;
	int mouseY = pt.y;
	float mousePos[2] = { mouseX, mouseY };

	// Exit UI
	float exitRatio[2] = { 0.9f, 0.95f };
	float exitUIOffset[2] = { 50.0f, 100.0f };

	float targetSize[2] = { 200, 200 };
	float hoveringSize[2] = { targetSize[0] - exitUIOffset[0],  targetSize[1] - exitUIOffset[1] };
	UIReact reactExit = MakeRect(winSize, hoveringSize, exitRatio);
	bool exitHoverTest = CheckMouseOnUI(reactExit, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, exitHoverTest, exitRatio);
	renderer.PrepareShaderUI(renderer.UIExitSRV);

	MenuActions action;
	if (InputManager::Input().IsClicked(MouseButton::Left) && exitHoverTest)
	{
		action.exit = true;
		MenuAction = action;
	}
	return MenuAction;
}

MenuActions MenuUI::DrawVictoryMenu(URenderer& renderer, HWND hWnd)
{
	MenuActions MenuAction;
	MenuAction.gameover = true;
	RECT rect;
	GetClientRect(hWnd, &rect);
	float winW = (float)(rect.right - rect.left);
	float winH = (float)(rect.bottom - rect.top);
	float winSize[2] = { winW, winH };
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	int mouseX = pt.x;
	int mouseY = pt.y;
	float mousePos[2] = { mouseX, mouseY };

	// Name UI
	float nameRatio[2] = { 0.5f, 0.5f };
	float targetSize[2] = { winW, winH };
	renderer.UpdateUIConstant(winSize, targetSize, false, nameRatio);
	renderer.PrepareShaderUI(renderer.UINameSRV);

	// Title UI
	float titleRatio[2] = { 0.5f, 0.3f };
	targetSize[0] = 500; targetSize[1] = 500;
	renderer.UpdateUIConstant(winSize, targetSize, true, titleRatio);
	renderer.PrepareShaderUI(renderer.UIVictorySRV);

	// Start UI
	float startRatio[2] = { 0.5f, 0.7f };
	float startUIOffset[2] = { 50.0f, 100.f };
	targetSize[0] = 200; targetSize[1] = 200;
	float hoveringSize[2] = { targetSize[0] - startUIOffset[0], targetSize[1] - startUIOffset[1] };
	UIReact reactStart = MakeRect(winSize, hoveringSize, startRatio);
	bool startHoverTest = CheckMouseOnUI(reactStart, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, startHoverTest, startRatio);
	renderer.PrepareShaderUI(renderer.UIStartSRV);

	// Menu UI
	float menuRatio[2] = { 0.5f, 0.8f };
	float menuUIOffset[2] = { 50.0f, 100.0f };
	targetSize[0] = 200; targetSize[1] = 150;
	hoveringSize[0] = targetSize[0] - menuUIOffset[0]; hoveringSize[1] = targetSize[1] - menuUIOffset[1];
	UIReact reactMenu = MakeRect(winSize, hoveringSize, menuRatio);
	bool menuHoverTest = CheckMouseOnUI(reactMenu, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, menuHoverTest, menuRatio);
	renderer.PrepareShaderUI(renderer.UIMenuSRV);
	// Exit UI
	float exitRatio[2] = { 0.9f, 0.95f };
	float exitUIOffset[2] = { 50.0f, 100.0f };
	targetSize[0] = 200; targetSize[1] = 200;
	hoveringSize[0] = targetSize[0] - exitUIOffset[0]; hoveringSize[1] = targetSize[1] - exitUIOffset[1];
	UIReact reactExit = MakeRect(winSize, hoveringSize, exitRatio);
	bool exitHoverTest = CheckMouseOnUI(reactExit, mouseX, mouseY);
	renderer.UpdateUIConstant(winSize, targetSize, exitHoverTest, exitRatio);
	renderer.PrepareShaderUI(renderer.UIExitSRV);
	MenuActions action;
	// ====== Ŭ�� ó�� ======
	if (InputManager::Input().IsClicked(MouseButton::Left) && startHoverTest)
	{
		action.start = true;
		MenuAction = action;
	}
	if (InputManager::Input().IsClicked(MouseButton::Left) && exitHoverTest)
	{
		action.exit = true;
		MenuAction = action;
		//NewController->bIsEnabled = true;
	}
	if (InputManager::Input().IsClicked(MouseButton::Left) && menuHoverTest)
	{
		action.menu = true;
		MenuAction = action;
	}
	return MenuAction;
}
