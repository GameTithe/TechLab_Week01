#pragma once

static const UINT UIVBMaxVerts = 1024;

struct UIInfo
{
	float Win[2];
	int IsHovering;
	float Padding;
};

struct UIReact
{
	float x0, x1;
	float y0, y1;
};

inline bool CheckMouseOnUI(const UIReact& TestUIReact, float x, float y)
{
	return (x < TestUIReact.x1 && x > TestUIReact.x0) && (y < TestUIReact.y1 && y > TestUIReact.y0);
}

inline UIReact MakeRect(const float winSize[2], const float targetSize[2], const float ratio[2])
{
	// 하드 코딩 TODO
	float w = targetSize[0] * (winSize[0] / 1024);
	float h = targetSize[1] * (winSize[1] / 1024);
	float cx = winSize[0] * ratio[0];
	float cy = winSize[1] * ratio[1];
	
	UIReact r;
	r.x0 = cx - w * 0.5f; r.y0 = cy - h * 0.5f;
	r.x1 = cx + w * 0.5f; r.y1 = cy + h * 0.5f;
	return r;

}