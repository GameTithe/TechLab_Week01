#pragma once

const int winWidth = 1024;
const int winHeight = 1024;

struct FPlayerInfo
{
	int att;
	float resolution[2];
	float iTime;
	float velocity[2];
};

struct FContactInfo
{
	float playerScale;
	float playerCenter;
	float Padding[2];
};
