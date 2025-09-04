#pragma once
#include <chrono>

#include "FVector.h"
#include "PlayerData.h"


// 객체의 속성을 정의하는 열거형 (Enum)
enum EAttribute
{
	WATER,
	FIRE,
	GRASS,
	NONE
};

// 게임 내 모든 기본 객체의 추상 기본 클래스
// 이 클래스는 순수 가상 함수를 포함하고 있어, 상속받는 모든 클래스가 반드시 이 함수들을 구현해야 합니다.
class UPrimitive
{
public:
	// 기본 생성자
	UPrimitive()
	{
	}

	// 가상 소멸자: 상속 클래스들이 올바르게 소멸되도록 보장합니다.
	virtual ~UPrimitive()
	{
	}
public:
	// 객체의 현재 위치를 반환
	virtual FVector GetLocation() const = 0;
	// 객체의 위치를 새로 설정
	virtual void SetLocation(FVector newLocation) = 0;

	// 객체의 현재 속도를 반환
	virtual FVector GetVelocity() const = 0;
	// 객체의 속도를 새로 설정
	virtual void SetVelocity(FVector newVelocity) = 0;

	// 객체의 질량을 반환
	virtual float GetMass() const = 0;
	// 객체의 반지름을 반환
	virtual float GetRadius() const = 0;
	// 객체의 자기력(자석 기능)을 반환
	virtual float GetMagnetic() const = 0;
	// 객체가 분열 상태인지 여부를 반환
	virtual bool GetDivide() const = 0;
	// 객체의 분열 상태를 설정
	virtual void SetDivide(bool newDivide) = 0;


	// 객체의 움직임을 처리하는 가상 함수
	virtual void Movement() = 0;
	// 객체의 속성(Attribute)을 반환
	virtual EAttribute GetAttribute() const = 0;
};


// 플레이어 캐릭터 클래스
// UPrimitive을 상속받아 모든 가상 함수를 구현합니다.
class UPlayer : public UPrimitive
{
public:
	// 플레이어 생성자
	UPlayer();

	// UPrimitive의 순수 가상 함수들을 모두 오버라이드하여 구현합니다.
	virtual FVector GetLocation() const override;
	virtual void SetLocation(FVector newLocation) override;

	virtual FVector GetVelocity() const override;
	virtual void SetVelocity(FVector newVelocity) override;

	virtual float GetMass() const override;
	virtual float GetRadius() const override;

	// GetAttribute 함수를 구현합니다.
	virtual EAttribute GetAttribute() const override;

	// GetMagnetic, GetDivide, SetDivide 함수는 기본적인 상태로 구현됩니다.
	virtual float GetMagnetic() const override;
	virtual bool GetDivide() const override;
	virtual void SetDivide(bool newDivide) override;

	// 플레이어의 이동 로직
	// 마우스 입력과 중력에 따른 힘을 적용합니다.
	void ApplyMouseForceAndGravity(FVector MouseWorldLocation, FVector CenterOfMass);
	// UPrimitive의 Movement 함수를 오버라이드하여 플레이어의 이동 로직을 처리합니다.
	virtual void Movement() override;

	// 플레이어의 속성(프로퍼티)을 조절하는 헬퍼 함수
	// 점수를 추가합니다.
	void AddScore(int amount);
	// 현재 점수를 반환합니다.
	int GetScore() const;
	// 반지름을 설정합니다.
	void SetRadius(float newRadius);

public:
	// 플레이어 상태를 나타내는 멤버 변수
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	int Score;
	EAttribute Attribute;
	FPlayerInfo PlayerInfo;
	const float MaxSpeed = 0.02f;
	float KnockedBackMaxSpeed;
	bool bIsKnockedBack = false;
	std::chrono::steady_clock::time_point knockbackStartTime;
};

// 화면 외부의 랜덤한 위치를 반환하는 전역 함수
FVector GetRandomLocationOusideScreen();
// 주어진 강도(Intensity)를 가진 랜덤 노이즈 벡터를 반환하는 전역 함수
FVector GetRandomNoiseVector(float Intensity);

// 적(Enemy) 객체 클래스
// UPrimitive을 상속받아 모든 가상 함수를 구현합니다.
class UEnemy : public UPrimitive
{
public:

	// 적 생성자
	UEnemy();

	// UPrimitive의 순수 가상 함수들을 모두 오버라이드하여 구현합니다.
	virtual FVector GetLocation() const override;
	virtual void SetLocation(FVector newLocation) override;
	virtual FVector GetVelocity() const override;
	virtual void SetVelocity(FVector newVelocity) override;
	virtual float GetMass() const override;
	virtual float GetRadius() const override;

	// GetAttribute 함수를 구현합니다.
	virtual EAttribute GetAttribute() const override;
	virtual float GetMagnetic() const override;
	virtual bool GetDivide() const override;
	virtual void SetDivide(bool newDivide) override;

	// UPrimitive의 Movement 함수를 오버라이드하여 적의 움직임(벽에 튕기는)을 처리합니다.
	virtual void Movement() override;

public:
	// 적의 상태를 나타내는 멤버 변수
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
};

// 먹이(Prey) 객체 클래스
// UPrimitive을 상속받아 모든 가상 함수를 구현합니다.
class UPrey : public UPrimitive
{
public:
	// 먹이 생성자
	UPrey();

	// UPrimitive의 순수 가상 함수들을 모두 오버라이드하여 구현합니다.
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

	// UPrimitive의 Movement 함수를 오버라이드하여 먹이의 움직임(움직이지 않음)을 처리합니다.
	virtual void Movement() override;

public:
	// 먹이의 상태를 나타내는 멤버 변수
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
	std::chrono::steady_clock::time_point ExpirationTime;
};