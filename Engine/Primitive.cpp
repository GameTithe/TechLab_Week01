#include <algorithm>
#include <windows.h>

#include "Primitive.h"
#include "UCamera.h"

// UPlayer 클래스 구현
UPlayer::UPlayer()
{
	// 1. �÷��̾� �Ӽ��� �������� ����
	Attribute = (EAttribute)(rand() % 3); // 0, 1, 2 �??�나�??�덤?�로 뽑아 ?�성?�로 지??

	Radius = 0.08f; // �ʱ� ũ��
	Mass = Radius * 10.0f;
	Location = FVector(0.0f, 0.0f, 0.0f); // ?�면 중앙?�서 ?�작
	Velocity = FVector(0.0f, 0.0f, 0.0f); // ?�도??마우?��? ?�르므�?0?�로 ?�작
	Score = 0;

	PlayerInfo.att = Attribute;
}

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
	// Empty implementation
}

void UPlayer::Movement()
{
	// 1. 넉백 상태인지 확인합니다.
	if (bIsKnockedBack)
	{
		// 2. 넉백이 시작된 후 5초가 지났는지 확인합니다.
		auto now = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - knockbackStartTime).count();

		if (elapsedTime >= 3)
		{
			// 3. 5초가 지났으면 넉백 상태를 해제하고, 속도를 0으로 초기화합니다.
			bIsKnockedBack = false;
			Velocity = FVector(0.0f, 0.0f, 0.0f);
		}
		else
		{
			// 4. 아직 5초가 안 지났다면, 속도에 따라 미끄러지게 합니다.
			Location += Velocity;

			// 5. 마찰력을 적용하여 서서히 멈추게 합니다. (0.98은 마찰 계수)
			Velocity.x *= 0.98f;
			Velocity.y *= 0.98f;
		}
	}

	// 6. 넉백 상태가 아닐 경우에만 마우스를 따라갑니다.
	if (!bIsKnockedBack)
	{
		extern HWND hWnd;

		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(hWnd, &mousePos);

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);

		float worldX = ((float)mousePos.x / clientRect.right) * 2.0f - 1.0f;
		float worldY = (-(float)mousePos.y / clientRect.bottom) * 2.0f + 1.0f;

		Location.x = worldX;
		Location.y = worldY;
	}
}

void UPlayer::AddScore(int amount)
{
	Score += amount;
}

int UPlayer::GetScore() const
{
	return Score;
}

void UPlayer::SetRadius(float newRadius)
{
	Radius = newRadius;
	// ?�기가 변?�면 질량??같이 ?�데?�트
	Mass = Radius * 10.0f;
}

// UEnemy 클래스 구현
UEnemy::UEnemy()
{
	Attribute = (EAttribute)(rand() % 3);
	Location = GetRandomLocationOusideScreen();

	const float enemySpeed = 0.001f;
	Velocity.x = (float)(rand() % 100 - 50) * enemySpeed;
	Velocity.y = (float)(rand() % 100 - 50) * enemySpeed;

	// TODO: Change FVector Instead Cam Location'
	if (UCamera::Main != nullptr)
	{
		FVector TargetLocation = UCamera::Main->Location + GetRandomNoiseVector(0.5f);
		Velocity = (TargetLocation - Location) * enemySpeed * 10;
	}
	else
	{
		Velocity = (FVector(0, 0, 0) - Location) * enemySpeed * 10;
	}

	Radius = ((rand() / (float)RAND_MAX)) * 0.1f + 0.03f;
	Mass = Radius * 10.0f;
}

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
	// Empty implementation
}

void UEnemy::Movement()
{
	Location += Velocity;

	/*if (Location.x > 1.0f - Radius || Location.x < -1.0f + Radius)
	{
		Velocity.x *= -1.0f;
	}
	if (Location.y > 1.0f - Radius || Location.y < -1.0f + Radius)
	{
		Velocity.y *= -1.0f;
	}*/
}

FVector UEnemy::GetRandomLocationOusideScreen()
{
	FVector Vector;
	do
	{
		Vector.x = (static_cast<float>(rand()) / RAND_MAX) * 4.0f - 2.0f; // -2 ~ 2
		Vector.y = (static_cast<float>(rand()) / RAND_MAX) * 4.0f - 2.0f; // -2 ~ 2
		Vector.z = 0.0f;
	}

	while ((Vector.x >= -1.0f && Vector.x <= 1.0f) &&
		   (Vector.y >= -1.0f && Vector.y <= 1.0f));

	return Vector;
}

FVector UEnemy::GetRandomNoiseVector(float Intensity)
{
	return FVector((rand() / (float)RAND_MAX) * Intensity, (rand() / (float)RAND_MAX) * Intensity, 0.0f);
}

// UPrey 클래스 구현
UPrey::UPrey()
{
	// 무작???�성, ?�치, ?�기 ?�정
	Attribute = (EAttribute)(rand() % 3);
	Velocity = FVector(0.0f, 0.0f, 0.0f); // ?�직이지 ?�으므�??�도??0
	Radius = ((rand() / (float)RAND_MAX)) * 0.05f + 0.02f;
	Mass = Radius * 10.0f;
	ExpirationTime = std::chrono::steady_clock::now() + std::chrono::seconds(15);
}

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
	// Empty implementation
}

void UPrey::Movement()
{
	// ?�무 코드???�음
}
