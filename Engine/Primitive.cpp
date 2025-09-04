#include <algorithm>
#include <windows.h>

#include "Primitive.h"
#include "UCamera.h"

// UPlayer 클래스 구현
UPlayer::UPlayer()
{
	// 1. 플레이어 속성 초기화
	// 0, 1, 2 중 하나를 랜덤으로 뽑아 속성을 지정합니다.
	Attribute = static_cast<EAttribute>(rand() % 3);

	// 초기 크기와 질량, 위치, 속도를 설정합니다.
	Radius = 0.08f;
	Mass = Radius * 10.0f;
	Location = FVector(0.0f, 0.0f, 0.0f); // 화면 중앙에서 시작
	Velocity = FVector(0.0f, 0.0f, 0.0f); // 마우스를 따라 움직이므로 초기 속도는 0으로 설정
	Score = 0;

	PlayerInfo.att = Attribute;
}

// UPrimitive 가상 함수 구현
FVector UPlayer::GetLocation() const
{
	return Location;
}

void UPlayer::SetLocation(FVector newLocation)
{
	Location = newLocation;
}

FVector UPlayer::GetVelocity() const
{
	return Velocity;
}

void UPlayer::SetVelocity(FVector newVelocity)
{
	Velocity = newVelocity;
}

float UPlayer::GetMass() const
{
	return Mass;
}

float UPlayer::GetRadius() const
{
	return Radius;
}

EAttribute UPlayer::GetAttribute() const
{
	return Attribute;
}

float UPlayer::GetMagnetic() const
{
	return 0.0f;
}

bool UPlayer::GetDivide() const
{
	return false;
}

void UPlayer::SetDivide(bool newDivide)
{
}

// 마우스 힘과 중력 적용
void UPlayer::ApplyMouseForceAndGravity(FVector MouseWorldLocation, FVector CenterOfMass)
{
	const float MouseAccStrength = 0.003f;			// 마우스 힘에 의한 가속도 크기
	const float KnockedBackAccStrength = 0.0009f;	// 넉백 상태에서의 마우스 힘에 의한 가속도 크기
	const float GravityStrength = 0.01f;			// 중력에 의한 가속도 크기

	// 마우스 방향으로의 가속도 계산
	FVector MouseAcc = MouseWorldLocation - CenterOfMass;
	if (MouseAcc.Magnitude() < 0.025f && !bIsKnockedBack)
	{
		// 마우스가 플레이어에 가까우면 멈춥니다.
		MouseAcc = FVector(0.0f, 0.0f, 0.0f);
		Velocity = FVector(0.0f, 0.0f, 0.0f);
	}
	else
	{
		// 넉백 상태에 따라 다른 가속도 크기를 적용합니다.
		MouseAcc *= bIsKnockedBack ? KnockedBackAccStrength : MouseAccStrength;
	}

 
	Velocity += MouseAcc;
	float baseLimit = bIsKnockedBack ? KnockedBackMaxSpeed : MaxSpeed;
	float dynamicLimit = baseLimit;

	// 2. 카메라 줌 레벨에 따라 최대 속도를 보정합니다.
	if (UCamera::Main && UCamera::Main->RenderScale > 0.01f)
	{
		dynamicLimit = baseLimit / UCamera::Main->RenderScale;
	}

	// 3. 보정된 '동적' 최대 속도를 적용하여 속도를 제한합니다.
	if (Velocity.Magnitude() > dynamicLimit)
	{
		Velocity.Normalize();
		Velocity *= dynamicLimit;
	}
}

// 플레이어 움직임 로직
void UPlayer::Movement()
{
	// 넉백 상태일 때는 물리 기반으로 미끄러집니다.
	if (bIsKnockedBack)
	{
		auto now = std::chrono::steady_clock::now();
		// 넉백 시작 시간으로부터 경과된 시간을 초 단위로 계산합니다.
		auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - knockbackStartTime).count();

		if (elapsedTime >= 3)
		{
			// 3초가 지나면 넉백 상태를 해제합니다.
			bIsKnockedBack = false;
		}
		else
		{
			// 넉백 상태에서는 추가적인 움직임 로직이 없습니다.
		}
	}

	Location += Velocity;
}

// 점수 추가
void UPlayer::AddScore(int amount)
{
	Score += amount;
}

// 점수 반환
int UPlayer::GetScore() const
{
	return Score;
}

// 반지름 설정
void UPlayer::SetRadius(float newRadius)
{
	Radius = newRadius;
	// 반지름이 변하면 질량도 같이 업데이트됩니다.
	Mass = Radius * 10.0f;
}

// 화면 밖의 랜덤한 위치를 반환
FVector GetRandomLocationOusideScreen()
{
	FVector Vector;
	do
	{
		// x, y 좌표를 -2.0f에서 2.0f 사이의 랜덤 값으로 설정합니다.
		Vector.x = (static_cast<float>(rand()) / RAND_MAX) * 4.0f - 2.0f;
		Vector.y = (static_cast<float>(rand()) / RAND_MAX) * 4.0f - 2.0f;
		Vector.z = 0.0f;
	} while ((Vector.x >= -1.0f && Vector.x <= 1.0f) &&
			 (Vector.y >= -1.0f && Vector.y <= 1.0f)); // 화면 안쪽이 아닌 위치를 찾을 때까지 반복합니다.

	return Vector;
}

// 지정된 강도의 랜덤 노이즈 벡터를 반환
FVector GetRandomNoiseVector(float Intensity)
{
	return FVector((rand() / static_cast<float>(RAND_MAX)) * Intensity, (rand() / static_cast<float>(RAND_MAX)) * Intensity, 0.0f);
}

// UEnemy 클래스 구현
UEnemy::UEnemy()
{
	// 속성을 랜덤으로 지정합니다.
	Attribute = static_cast<EAttribute>(rand() % 3);
	// 화면 밖의 랜덤한 위치를 계산합니다.
	FVector RandomLocationOusideScreen = GetRandomLocationOusideScreen();
	// 카메라 스케일에 맞게 적의 초기 위치를 설정합니다.
	Location = RandomLocationOusideScreen / UCamera::Main->RenderScale + UCamera::Main->Location;

	const float enemySpeed = 0.001f;
	// 초기 속도를 랜덤 노이즈와 초기 위치를 기반으로 설정합니다.
	Velocity = (GetRandomNoiseVector(0.5f) - RandomLocationOusideScreen) * enemySpeed * 10;

	// 반지름과 질량을 랜덤하게 설정합니다.
	Radius = ((rand() / static_cast<float>(RAND_MAX))) * 0.1f + 0.03f;
	Mass = Radius * 10.0f;
}

// UPrimitive 가상 함수 구현
FVector UEnemy::GetLocation() const
{
	return Location;
}

void UEnemy::SetLocation(FVector newLocation)
{
	Location = newLocation;
}

FVector UEnemy::GetVelocity() const
{
	return Velocity;
}

void UEnemy::SetVelocity(FVector newVelocity)
{
	Velocity = newVelocity;
}

float UEnemy::GetMass() const
{
	return Mass;
}

float UEnemy::GetRadius() const
{
	return Radius;
}

EAttribute UEnemy::GetAttribute() const
{
	return Attribute;
}

float UEnemy::GetMagnetic() const
{
	return 0.0f;
}

bool UEnemy::GetDivide() const
{
	return false;
}

void UEnemy::SetDivide(bool newDivide)
{
	// 비어있는 구현
}

// 적의 움직임 로직
void UEnemy::Movement()
{
	Location += Velocity;
}

// UPrey 클래스 구현
UPrey::UPrey()
{
	// 속성을 NONE으로 지정합니다.
	Attribute = EAttribute::NONE;
	// 먹이는 움직이지 않으므로 속도를 0으로 설정합니다.
	Velocity = FVector(0.0f, 0.0f, 0.0f);
	// 카메라 스케일에 맞게 먹이의 초기 위치를 설정합니다.
	Location = GetRandomLocationOusideScreen() / UCamera::Main->RenderScale + UCamera::Main->Location;
	// 반지름과 질량을 랜덤하게 설정합니다.
	Radius = ((rand() / static_cast<float>(RAND_MAX))) * 0.01f + 0.02f;
	Mass = Radius * 10.0f;
	// 생성 후 15초 뒤에 사라지도록 만료 시간을 설정합니다.
	ExpirationTime = std::chrono::steady_clock::now() + std::chrono::seconds(15);
}

// UPrimitive 가상 함수 구현
FVector UPrey::GetLocation() const
{
	return Location;
}

void UPrey::SetLocation(FVector newLocation)
{
	Location = newLocation;
}

FVector UPrey::GetVelocity() const
{
	return Velocity;
}

void UPrey::SetVelocity(FVector newVelocity)
{
	Velocity = newVelocity;
}

float UPrey::GetMass() const
{
	return Mass;
}

float UPrey::GetRadius() const
{
	return Radius;
}

EAttribute UPrey::GetAttribute() const
{
	return Attribute;
}

float UPrey::GetMagnetic() const
{
	return 0.0f;
}

bool UPrey::GetDivide() const
{
	return false;
}

void UPrey::SetDivide(bool newDivide)
{
}

void UPrey::Movement()
{
}