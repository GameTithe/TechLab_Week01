#pragma once
#include <chrono>

#include "FVector.h"
#include "PlayerData.h"


enum EAttribute
{
	WATER, 
	FIRE,  
	GRASS,
	NONE
};

class UPrimitive
{
public:
	UPrimitive()
	{
	}

	virtual ~UPrimitive()
	{
	}
public:
	virtual FVector GetLocation() const = 0;
	virtual void SetLocation(FVector newLocation) = 0;

	virtual FVector GetVelocity() const = 0;
	virtual void SetVelocity(FVector newVelocity) = 0;

	virtual float GetMass() const = 0;
	virtual float GetRadius() const = 0;
	virtual float GetMagnetic() const = 0;
	virtual bool GetDivide() const = 0;
	virtual void SetDivide(bool newDivide) = 0;


	virtual void Movement() = 0;
	virtual EAttribute GetAttribute() const = 0;
};


class UPlayer : public UPrimitive
{
public:
	// ?�성?? ?�레?�어 ?�성 ???�출??
	UPlayer();

	// UPrimitive??규칙(?�수 가???�수)???�라 모든 ?�수�?구현
	virtual FVector GetLocation() const override;
	virtual void SetLocation(FVector newLocation) override;

	virtual FVector GetVelocity() const override;
	virtual void SetVelocity(FVector newVelocity) override;

	virtual float GetMass() const override;
	virtual float GetRadius() const override;
	// 규칙???�라 GetAttribute ?�수�?구현
	virtual EAttribute GetAttribute() const override;
	// ?�용?��? ?�는 기능?��? 기본 ?�태�?구현
	virtual float GetMagnetic() const override;
	virtual bool GetDivide() const override;
	virtual void SetDivide(bool newDivide) override;

	// ?�레?�어???�심 로직: 마우?��? ?�라 ?�직임
	virtual void Movement() override;

	// ?�기?� ?�수�?조절?�는 ?�로???�수??
	void AddScore(int amount);
	int GetScore() const;
	void SetRadius(float newRadius);


public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	int Score;
	EAttribute Attribute;
	FPlayerInfo PlayerInfo;
	bool bIsKnockedBack = false;
	std::chrono::steady_clock::time_point knockbackStartTime;
};

FVector GetRandomLocationOusideScreen();
FVector GetRandomNoiseVector(float Intensity);

class UEnemy : public UPrimitive
{
public:

	UEnemy();

	// UPrimitive??규칙???�라 모든 ?�수�?구현

	virtual FVector GetLocation() const override;
	virtual void SetLocation(FVector newLocation) override;
	virtual FVector GetVelocity() const override;
	virtual void SetVelocity(FVector newVelocity) override;
	virtual float GetMass() const override;
	virtual float GetRadius() const override;
	// 규칙???�라 GetAttribute ?�수�?구현
	virtual EAttribute GetAttribute() const override;
	virtual float GetMagnetic() const override;
	virtual bool GetDivide() const override;
	virtual void SetDivide(bool newDivide) override;

	// ENEMY???�직임: 기존 UBall처럼 벽에 ?��?
	virtual void Movement() override;

public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
};

class UPrey : public UPrimitive
{
public:
	// ?�성?? PREY ?�성 ???�출??
	UPrey();

	// UPrimitive??규칙???�라 모든 ?�수�?구현
	virtual FVector GetLocation() const override;
	virtual void SetLocation(FVector newLocation) override;
	virtual FVector GetVelocity() const override;
	virtual void SetVelocity(FVector newVelocity) override;
	virtual float GetMass() const override;
	virtual float GetRadius() const override;
	virtual EAttribute GetAttribute() const override;
	virtual float GetMagnetic() const override;
	virtual bool GetDivide() const override;
	virtual void SetDivide(bool newDivide) override;

	// PREY???�직임: ?�직이지 ?�음
	virtual void Movement() override;

public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
	std::chrono::steady_clock::time_point ExpirationTime;
};
