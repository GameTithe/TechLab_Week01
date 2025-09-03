#pragma once
#include <windows.h>
#include <windowsx.h>

enum class MouseButton { Left = 0, Right = 1, Middle = 2, Count };

struct InputManager
{
	static InputManager& Input() { static InputManager Instance; return Instance; }

	void BeginFrame()
	{
		for (int i = 0; i < (int)MouseButton::Count; ++i)
		{
			prevMouse[i] = curMouse[i];
		}
	}
	void ProcessMessage(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_MOUSEMOVE:
			mouseX = GET_X_LPARAM(lParam);
			mouseY = GET_Y_LPARAM(lParam);
			break;
		case WM_LBUTTONDOWN: 
			curMouse[(int)MouseButton::Left] = true;  
			break;
		case WM_LBUTTONUP:   
			curMouse[(int)MouseButton::Left] = false; 
			break;
		case WM_RBUTTONDOWN: 
			curMouse[(int)MouseButton::Right] = true;  
			break;
		case WM_RBUTTONUP:   
			curMouse[(int)MouseButton::Right] = false; 
			break;
		}
	 }
	bool IsDown(MouseButton b)		const { return curMouse[(int)b]; }
	bool IsClicked(MouseButton b)	const { return !curMouse[(int)b] && prevMouse[(int)b]; }

public:
	bool curMouse[(int)MouseButton::Count] = { false };
	bool prevMouse[(int)MouseButton::Count] = { false };

	int mouseX = 0, mouseY = 0;
};