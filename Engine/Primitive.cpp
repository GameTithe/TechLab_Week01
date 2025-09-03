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
	// 넉백 상태일 때는 물리 기반으로 미끄러집니다.
	if (bIsKnockedBack)
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - knockbackStartTime).count();

		if (elapsedTime >= 3)
		{
			bIsKnockedBack = false;
			Velocity = FVector(0.0f, 0.0f, 0.0f);
		}
		else
		{
			// 위치 업데이트
			Location += Velocity;
			// 마찰력
			Velocity.x *= 0.98f;
			Velocity.y *= 0.98f;
		}
		return; // 넉백 상태일 때는 아래의 조작 로직을 실행하지 않음
	}

	// ==========================================================
	// ▼▼▼▼▼ 핵심 수정: 속도 기반 이동 로직 ▼▼▼▼▼
	// ==========================================================
	extern HWND hWnd;

	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(hWnd, &mousePos);

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	// 1. 마우스 위치를 '월드 좌표'로 변환합니다.
	float cameraX = ((float)mousePos.x / clientRect.right) * 2.0f - 1.0f;
	float cameraY = (-(float)mousePos.y / clientRect.bottom) * 2.0f + 1.0f;
	FVector mouseWorldPos = FVector(0.f, 0.f, 0.f);

	if (UCamera::Main)
	{
		mouseWorldPos = UCamera::ConvertToWorldSpaceLocation(FVector(cameraX, cameraY, 0.0f));
	}

	// 2. 플레이어 위치에서 마우스 월드 위치를 향하는 '방향 벡터'를 구합니다.
	FVector moveDirection = mouseWorldPos - Location;
	float dist2 = moveDirection.x * moveDirection.x + moveDirection.y * moveDirection.y;

	// 3. 마우스가 플레이어와 매우 가깝지 않을 때만 움직이도록 '데드존'을 설정합니다.
	if (dist2 > 0.0001f) // 매우 작은 값보다 클 때만
	{
		// 방향 벡터를 정규화(길이를 1로 만듦)합니다.
		float dist = sqrt(dist2);
		moveDirection.x /= dist;
		moveDirection.y /= dist;

		// 4. 이 방향으로 일정한 속도를 설정합니다.
		const float playerSpeed = 0.015f; // 플레이어 속도. 이 값을 조절해 보세요.
		Velocity = moveDirection * playerSpeed;
	}
	else
	{
		// 마우스가 플레이어 위에 있으면 멈추도록 속도를 0으로 설정합니다.
		Velocity = FVector(0.0f, 0.0f, 0.0f);
	}

	// 5. 최종적으로 계산된 속도를 위치에 더해 플레이어를 움직입니다.
	Location += Velocity;
	// ==========================================================
	// ▲▲▲▲▲ 핵심 수정 끝 ▲▲▲▲▲
	// ==========================================================
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
	Attribute = EAttribute::NONE;
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
