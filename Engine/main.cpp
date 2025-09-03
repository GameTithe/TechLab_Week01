#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <vector>
#include <chrono>

// D3D libraries
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")	

// D3D headers
#include <d3d11.h>
#include <d3dcompiler.h>	

//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"


// UI 
#include "MenuUI.h"
#include "UIInfo.h"
#include <iostream> 
#include <ctime>    
#include <cstdlib>   

HWND hWnd = nullptr;
 
//Manager
#include "InputManager.h"

#include "PlayerData.h"
#include "FVector.h"
#include "UCamera.h"

// Sound
#include "SoundManager.h"

#include "VertexInfo.h" 
#include "URenderer.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
 
static bool MouseClicked = false;
 
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	InputManager::Input().ProcessMessage(hWnd, message, wParam, lParam);


	switch (message)
	{
	case WM_ACTIVATE: // â�� Ȱ��ȭ ���°� ����� ��
		if (LOWORD(wParam) != WA_INACTIVE) // Ȱ��ȭ�� ��
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			ClientToScreen(hWnd, (LPPOINT)&rect.left); // ���-���� ��ǥ ��ȯ
			ClientToScreen(hWnd, (LPPOINT)&rect.right); // �ϴ�-���� ��ǥ ��ȯ
			ClipCursor(&rect); // Ŀ���� �ش� �簢�� ������ ����
		}
		else // ��Ȱ��ȭ�� ��
		{
			ClipCursor(NULL); // Ŀ�� ���α� ����
		}
		break;

	case WM_DESTROY:
		// signal the the app should quit
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
} 
 
enum EAttribute
{
	WATER,  
	FIRE,   
	GRASS   
};
 
bool CheckWin(EAttribute playerAttribute, EAttribute otherAttribute)
{
	if (playerAttribute == WATER && otherAttribute == FIRE) return true;
	if (playerAttribute == FIRE && otherAttribute == GRASS) return true;
	if (playerAttribute == GRASS && otherAttribute == WATER) return true;
	return false;
}

inline float Dot(const FVector& v1, const FVector& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

//sphere
#include "Sphere.h" 

class UPlayer; 

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

class UBall : public UPrimitive
{
public:
	UBall()
	{
		++TotalBalls;
		float randPos = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
		Location = FVector(randPos, randPos, 0.0f);

		const float ballSpeed = 0.001f;
		Velocity.x = (float)(rand() % 100 - 50) * ballSpeed;
		Velocity.y = (float)(rand() % 100 - 50) * ballSpeed;

		Radius = ((rand() / (float)RAND_MAX)) * 0.2f; // 0.0f~ 0.2f 
		Radius = Radius < 0.05f ? 0.05f : Radius;	  // 0.05 ~ 0.2

		Mass = Radius * 10.0f;

		float randMagnetic = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
		int sign = randMagnetic > 0 ? 1 : -1;
		Magnetic = Mass * sign;

		bDivide = false;
	}
	UBall(float radius) : Radius(radius)
	{
		++TotalBalls;
		float randPos = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
		Location = FVector(randPos, randPos, 0.0f);

		const float ballSpeed = 0.001f;
		Velocity.x = (float)(rand() % 100 - 50) * ballSpeed;
		Velocity.y = (float)(rand() % 100 - 50) * ballSpeed;

		Mass = Radius * 10.0f;

		float randMagnetic = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
		int sign = randMagnetic > 0 ? 1 : -1;
		Magnetic = Mass * sign;

		bDivide = false;
	}
	virtual ~UBall()
	{
		--TotalBalls;
	}

public:
	virtual FVector GetLocation() const override
	{
		return Location;
	}
	virtual void SetLocation(FVector newLocation)
	{
		Location = newLocation;
	}

	virtual FVector GetVelocity() const override
	{
		return Velocity;
	}

	virtual void SetVelocity(FVector newVelocity) override
	{
		Velocity = newVelocity;
	}
	virtual float GetMass() const override
	{
		return Mass;
	}

	virtual float GetRadius() const override
	{
		return Radius;
	}
	virtual float GetMagnetic() const
	{
		return Magnetic;
	}
	virtual bool GetDivide() const override
	{
		return bDivide;
	}
	virtual void SetDivide(bool newDivide) override
	{
		bDivide = newDivide;
	}
	virtual void Movement() override
	{
		const float leftBorder = -1.0f;
		const float rightBorder = 1.0f;
		const float topBorder = 1.0f;
		const float bottomBorder = -1.0f;

		Location.x += Velocity.x;
		Location.y += Velocity.y;

		if (Location.x > rightBorder - Radius)
		{
			Velocity.x *= -1;
		}
		else if (Location.x < leftBorder + Radius)
		{
			Velocity.x *= -1;
		}

		if (Location.y > topBorder - Radius)
		{
			Velocity.y *= -1;
		}
		else if (Location.y < bottomBorder + Radius)
		{
			Velocity.y *= -1;
		}


		if (Location.x > rightBorder - Radius)
		{
			Location.x = rightBorder - Radius;
		}
		if (Location.x < leftBorder + Radius)
		{
			Location.x = leftBorder + Radius;
		}

		if (Location.y > topBorder - Radius)
		{
			Location.y = topBorder - Radius;
		}
		if (Location.y < bottomBorder + Radius)
		{
			Location.y = bottomBorder + Radius;

		}
	}


public:
	static ID3D11Buffer* vertexBufferSphere;
	static int TotalBalls;

	FVector Location;
	FVector Velocity;
	float Radius = 0.1f;
	float Mass;

	float Magnetic;
	bool bDivide;
};
class UPlayer : public UPrimitive
{
public:
	// ?�성?? ?�레?�어 ?�성 ???�출??
	UPlayer()
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

	// UPrimitive??규칙(?�수 가???�수)???�라 모든 ?�수�?구현
	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }

	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }

	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// 규칙???�라 GetAttribute ?�수�?구현
	virtual EAttribute GetAttribute() const override { return Attribute; }
	// ?�용?��? ?�는 기능?��? 기본 ?�태�?구현
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// ?�레?�어???�심 로직: 마우?��? ?�라 ?�직임
	virtual void Movement() override
	{
		extern HWND hWnd; // WinMain??hWnd�??��??�서 참조

		POINT mousePos;
		GetCursorPos(&mousePos); // 마우?�의 ?�크�?좌표�??�음
		ScreenToClient(hWnd, &mousePos); // ?�크�?좌표�??�로그램 �??��? 좌표�?변??

		RECT clientRect;
		GetClientRect(hWnd, &clientRect); // ?�로그램 창의 ?�기�??�음

		// �??��? 좌표(e.g., 0~1024)�?게임 ?�드 좌표(-1.0 ~ 1.0)�?변??
		float worldX = ((float)mousePos.x / clientRect.right) * 2.0f - 1.0f;
		float worldY = (-(float)mousePos.y / clientRect.bottom) * 2.0f + 1.0f;

		Location.x = worldX;
		Location.y = worldY;
	}

	// ?�기?� ?�수�?조절?�는 ?�로???�수??
	void AddScore(int amount) { Score += amount; }
	int GetScore() const { return Score; }
	void SetRadius(float newRadius)
	{
		Radius = newRadius;
		// ?�기가 변?�면 질량??같이 ?�데?�트
		Mass = Radius * 10.0f;
	}


public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	int Score;
	EAttribute Attribute;
	FPlayerInfo PlayerInfo;
};

FVector GetRandomNoiseVector(float Intensity)
{
	return FVector((rand() / (float)RAND_MAX) * Intensity, (rand() / (float)RAND_MAX) * Intensity, 0.0f);
}

class UEnemy : public UPrimitive
{
public:

	UEnemy()
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

	// UPrimitive??규칙???�라 모든 ?�수�?구현

	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }
	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }
	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// 규칙???�라 GetAttribute ?�수�?구현
	virtual EAttribute GetAttribute() const override { return Attribute; }
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// ENEMY???�직임: 기존 UBall처럼 벽에 ?��?
	virtual void Movement() override
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

	FVector GetRandomLocationOusideScreen()
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
	UPrey()
	{
		// 무작???�성, ?�치, ?�기 ?�정
		Attribute = (EAttribute)(rand() % 3);
		Velocity = FVector(0.0f, 0.0f, 0.0f); // ?�직이지 ?�으므�??�도??0
		Radius = ((rand() / (float)RAND_MAX)) * 0.05f + 0.02f;
		Mass = Radius * 10.0f;
		ExpirationTime = std::chrono::steady_clock::now() + std::chrono::seconds(15);
	}

	// UPrimitive??규칙???�라 모든 ?�수�?구현
	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }
	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }
	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	virtual EAttribute GetAttribute() const override { return Attribute; }
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// PREY???�직임: ?�직이지 ?�음
	virtual void Movement() override
	{
		// ?�무 코드???�음
	}

public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
	std::chrono::steady_clock::time_point ExpirationTime;
};

//int UBall::TotalBalls = 0;
//ID3D11Buffer* UBall::vertexBufferSphere = nullptr; // static 멤버 변??초기??


class UController
{
public:
	UPlayer** PlayerCells = nullptr;
	int MaxCount = 0;
	int Count = 0;
	bool bIsEnabled = false;

public:
	UController(int MaxCount)
	{
		this->MaxCount = MaxCount;
		Count = 0;
		PlayerCells = new UPlayer * [MaxCount];
		for (int i = 0; i < MaxCount; ++i)
		{
			PlayerCells[i] = nullptr;
		}
		bIsEnabled = false;
	}

	~UController()
	{
		for (int i = 0; i < Count; ++i)
		{
			delete PlayerCells[i];
		}
		delete[] PlayerCells;
	}

	bool TryGetCenterOfMass(FVector& CenterOfMass)
	{
		if (Count == 0)
		{
			return false;
		}

		float CenterX = 0.0f;
		float CenterY = 0.0f;
		float TotalMass = 0.0f;

		for (int i = 0; i < Count; ++i)
		{
			// Prevent NRE
			UPlayer* Cell = PlayerCells[i];
			if (!Cell)
			{
				continue;
			}
			float Mass = Cell->GetMass();
			FVector Location = Cell->GetLocation();
			CenterX += Location.x * Mass;
			CenterY += Location.y * Mass;
		}

		// Fallback: if TotalMass is too small, calculate simple average
		if (TotalMass < 1e-6)
		{
			CenterX = CenterY = 0.0f;
			for (int i = 0; i < Count; ++i)
			{
				// Prevent NRE
				UPlayer* Cell = PlayerCells[i];
				if (!Cell)
				{
					continue;
				}

				FVector Location = Cell->GetLocation();
				CenterX += Location.x;
				CenterY += Location.y;
			}
			CenterOfMass = FVector(CenterX / Count, CenterY / Count, 0.0f);
			return true;
		}

		CenterOfMass = FVector(CenterX / TotalMass, CenterY / TotalMass, 0.0f);
		return true;
	}

	// Calculate Move Vector and Call Movement for each Player Cell
	void Move(const FVector& CursorWorldCoord)
	{
		FVector CenterOfMass;
		if (!TryGetCenterOfMass(CenterOfMass))
		{
			return;
		}

		FVector MoveVector = CursorWorldCoord - CenterOfMass;
		MoveVector.Normalize();

		for (int i = 0; i < Count; ++i)
		{
			UPlayer* Cell = PlayerCells[i];
			if (!Cell)
			{
				continue;
			}
			}
	}

	// Enroll New Player Cell to Controller (initialize or split)
	void Enroll(UPlayer* NewCell)
	{
		if (Count < MaxCount)
		{
			PlayerCells[Count] = NewCell;
			++Count;
		}
	}

	// Remove Player Cell from Controller (death or merge)
	void Release(UPlayer* Cell)
	{
		for (int i = 0; i < Count; ++i)
		{
			if (PlayerCells[i] == Cell)
			{
				delete PlayerCells[i];
				PlayerCells[i] = PlayerCells[Count - 1];
				PlayerCells[Count - 1] = nullptr;
				--Count;
				break;
			}
		}
	}
};
struct Merge
{
	int indexA;
	int indexB;
};

class FPrimitiveVector
{
public:
	// ?�레?�어 객체???�게 ?�근?�기 ?�한 ?�인??
	UPlayer* Player = nullptr;

	FPrimitiveVector()
	{
		Capacity = 10;
		Size = 0;

		primitives = new UPrimitive * [Capacity];
	}
	~FPrimitiveVector()
	{
		for (int i = 0; i < Size; i++)
		{
			delete primitives[i];
		}
		delete[] primitives;
	}

	void push_back(UPrimitive* primitive)
	{
		if (Size >= Capacity)
		{
			ReSize();
		}

		// 만약 추�??�는 객체가 ?�레?�어?�면, Player ?�인?�에 ?�??
		// dynamic_cast??UPrimitive*�?UPlayer*�??�전?�게 변???�도
		UPlayer* playerCandidate = dynamic_cast<UPlayer*>(primitive);
		if (playerCandidate != nullptr)
		{
			Player = playerCandidate;
		}

		primitives[Size++] = primitive;
	}

	void pop_random()
	{
		if (Size > 1)
		{
			int index = rand() % Size;
			delete primitives[index];

			primitives[index] = primitives[Size - 1];
			primitives[Size - 1] = nullptr;
			Size--;
		}
	}

	void pop_back()
	{
		if (Size > 1)
		{
			delete primitives[Size - 1];
			primitives[Size - 1] = nullptr;
			Size--;
		}
	}
	void RemoveAt(int index)
	{
		// ?�효?��? ?��? ?�덱?�면 즉시 종료
		if (index < 0 || index >= Size)
		{
			return;
		}

		// 객체 메모�??�제
		delete primitives[index];

		// 마�?�??�소�??�재 ?�치�??�동 (가??빠른 ?�거 방법)
		primitives[index] = primitives[Size - 1];
		primitives[Size - 1] = nullptr;
		Size--;
	}

	void ReSize()
	{
		Capacity = Size * 2;

		UPrimitive** newPrimitives = new UPrimitive * [Capacity];
		for (int i = 0; i < Size; ++i)
		{
			newPrimitives[i] = primitives[i];
		}
		delete[] primitives;

		primitives = newPrimitives;
	}

	int size() const // 캡슐?��? ?�비해???�수�??�근
	{
		return Size;
	}

	UPrimitive* operator[](int index)
	{
		if (index < 0 || index >= Size)
		{
			return nullptr;
		}
		return primitives[index];
	}

	void ProcessGameLogic()
	{
		/** 플레이어가 없으면 아무것도 하지 않음 */
		if (Player == nullptr) return;

		/** 배열을 거꾸로 순회해야 객체 삭제 시에도 안전함 */
		for (int i = Size - 1; i >= 0; --i)
		{
			/** 자기 자신(플레이어)과는 충돌 검사를 하지 않음 */
			if (primitives[i] == Player)
			{
				continue;
			}

			UPrimitive* other = primitives[i];
			float dist2 = FVector::Distance2(Player->GetLocation(), other->GetLocation());
			float minDist = Player->GetRadius() + other->GetRadius();

			/** 충돌했다면 */
			if (dist2 < minDist * minDist)
			{
				/** 콜리전 발생 시 소리 재생 */
				// TODO: 나중에 PreyEat랑 구분해야함
				//USoundManager::PlayCollideSound();

				EAttribute playerAttr = Player->GetAttribute();
				EAttribute otherAttr = other->GetAttribute();

				if (CheckWin(playerAttr, otherAttr)) /** 이기는 경우 */
				{
					USoundManager::Collide();

					Player->AddScore(10);
					Player->SetRadius(Player->GetRadius() + 0.005f);
					RemoveAt(i); /** 상대 객체를 제거 */
				}
				else if (playerAttr != otherAttr) /** 지는 경우 (비기지 않은 경우) */
				{
					USoundManager::PreyEat();

					Player->AddScore(-5);
					Player->SetRadius(Player->GetRadius() - 0.005f);
					RemoveAt(i); /** 상대 객체를 제거 */
				}

				else if (playerAttr == otherAttr) // ����� ��
				{
					RemoveAt(i);
				}
			}
		}
	}
	void RespawnAllPrey()
	{
		// 1. 기존에 남아있는 모든 PREY를 찾아서 삭제
		for (int i = Size - 1; i >= 0; i--)
		{
			// dynamic_cast를 사용해 UPrimitive가 UPrey인지 확인
			if (dynamic_cast<UPrey*>(primitives[i]))
			{
				RemoveAt(i);
			}
		}

		// 2. 새로운 PREY들을 생성
		for (int i = 0; i < 15; ++i) // 15개의 PREY를 새로 생성
		{
			push_back(new UPrey());
		}
	}

	// PREY의 수명과 재생성 여부를 매 프레임마다 확인하는 함수
	void UpdatePreyLifecycle()
	{
		int waterPreyCount = 0;
		int firePreyCount = 0;
		int grassPreyCount = 0;
		bool needsRespawn = false;

		// 1. 수명이 다한 PREY를 삭제하고, 현재 속성별 개수를 셉니다.
		for (int i = Size - 1; i >= 0; i--)
		{
			// dynamic_cast를 사용해 UPrimitive가 UPrey인지 확인
			UPrey* prey = dynamic_cast<UPrey*>(primitives[i]);
			if (prey) // UPrey가 맞다면
			{
				// 수명이 다했는지 체크
				if (std::chrono::steady_clock::now() > prey->ExpirationTime)
				{
					RemoveAt(i); // 수명이 다했으면 삭제
					continue;    // 다음 객체로
				}

				// 살아남은 PREY의 속성별 개수 카운트
				switch (prey->GetAttribute())
				{
				case WATER: waterPreyCount++; break;
				case FIRE:  firePreyCount++; break;
				case GRASS: grassPreyCount++; break;
				}
			}
		}

		// 2. 한 가지 속성이라도 개수가 0인지 확인합니다.
		if (waterPreyCount == 0 || firePreyCount == 0 || grassPreyCount == 0)
		{
			needsRespawn = true;
		}

		// 3. 조건이 만족되면 모든 PREY를 새로 생성합니다.
		if (needsRespawn)
		{
			RespawnAllPrey();
		}
	}
	 
	// 거리 ?�곱??반비례
	void MagneticForce()
	{
		for (int i = 0; i < Size; ++i)
		{
			FVector totalMagneticForce = FVector(0, 0, 0);
			UPrimitive* a = primitives[i];
			for (int j = 0; j < Size; ++j)
			{
				if (i == j) continue;

				UPrimitive* b = primitives[j];

				float k = 0.001f; //?�치 ?�정???�해???�의�?조정
				float q1 = a->GetMagnetic();
				float q2 = b->GetMagnetic();
				float dist2 = FVector::Distance2(a->GetLocation(), b->GetLocation());

				dist2 = dist2 == 0 ? 1e-5 : dist2;
				float force = q1 * q2 * k / dist2;

				FVector normal = a->GetLocation() - b->GetLocation();
				//normal.Normalize(); // normalize?�주�??�무 커질 ???�으???�스

				totalMagneticForce += normal * force;
			}
			FVector acceleration = totalMagneticForce / a->GetMass();

			a->SetVelocity(a->GetVelocity() + acceleration);
		}
	}

	// ?�면 ?�역 ?�에 ?�는지 체크?�는 ?�수
	bool IsInRenderArea(const FVector& renderedLocation, float renderedRadius,
						float minX = -2.0f, float maxX = 2.0f,
						float minY = -2.0f, float maxY = 2.0f) const
	{
		return (renderedLocation.x + renderedRadius >= minX &&
				renderedLocation.x - renderedRadius <= maxX &&
				renderedLocation.y + renderedRadius >= minY &&
				renderedLocation.y - renderedRadius <= maxY);
	}

	// 보이??객체?� 보이지 ?�는 객체�?분류?�는 ?�수
	void ClassifyBorder(UCamera* camera,
						std::vector<int>& InSideIndices,
						std::vector<int>& OutSideIndices)
	{
		InSideIndices.clear();
		OutSideIndices.clear();

		for (int i = 0; i < Size; i++)
		{
			FVector renderedLocation = camera->ConvertToCameraSpaceLocation(primitives[i]->GetLocation());
			float renderedRadius = camera->ConvertToCameraSpaceRadius(primitives[i]->GetRadius());

			if (IsInRenderArea(renderedLocation, renderedRadius))
			{
				InSideIndices.push_back(i);
			}
			else
			{
				OutSideIndices.push_back(i);
			}
		}
	}

	// 보이지 ?�는 객체?�을 ??��?�는 ?�수
	void RemoveOutsidePrimitives(const std::vector<int>& invisibleIndices)
	{
		// ?�에?��?????�� (?�덱??변??방�?)
		for (int i = invisibleIndices.size() - 1; i >= 0; i--)
		{
			int deleteIndex = invisibleIndices[i];
			if (deleteIndex < 0 || deleteIndex >= Size) continue; // ?�전??체크

			delete primitives[deleteIndex];

			// 마�?�??�소�???��???�치�??�동
			primitives[deleteIndex] = primitives[Size - 1];
			primitives[Size - 1] = nullptr;
			Size--;
		}
	}
	FVector FindSafeSpawnLocation(float objectRadius)
	{
		FVector spawnLocation;
		bool isSafe = false;
		int maxAttempts = 50; // 위치를 찾기 위해 최대 50번 시도

		for (int attempt = 0; attempt < maxAttempts; ++attempt)
		{
			// 1. 화면 안쪽(-1.0 ~ 1.0)에 랜덤 위치 생성
			spawnLocation.x = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
			spawnLocation.y = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
			spawnLocation.z = 0.0f;

			isSafe = true;

			// 2. 현재 존재하는 모든 객체와 겹치는지 검사
			for (int i = 0; i < Size; ++i)
			{
				if (primitives[i] == nullptr) continue;

				float dist2 = FVector::Distance2(spawnLocation, primitives[i]->GetLocation());
				float minDist = objectRadius + primitives[i]->GetRadius();

				if (dist2 < minDist * minDist) // 겹쳤다면
				{
					isSafe = false; // 안전하지 않음
					break;          // 다음 시도로 넘어감
				}
			}

			if (isSafe) // 안전한 위치를 찾았다면
			{
				return spawnLocation;
			}
		}

		// 50번 시도해도 못 찾았다면, 그냥 마지막 위치를 반환 (게임이 멈추는 것을 방지)
		return spawnLocation;
	}

public:

	UPrimitive** primitives = nullptr;
	int Capacity = 0;
	int Size = 0;

	Merge mergeList[1024] = { -1, -1 };// 1충돌??2개의 구�? ?�요?�다�??�면, 1024�??�넉??것으�??�상, 만약 1024번을 ?�기�?그냥 충돌�?처리
	int mergeCount = 0;
};

//int DesireNumberOfBalls = UBall::TotalBalls;

/** Clock type definition */
using Clock = std::chrono::steady_clock;

class UEnemySpawner
{
public:
	UEnemySpawner(int BaseInterval = 300, int RandomInterval = 300) : BaseSpawnIntervalMs(BaseInterval), RandomSpawnIntervalMs(RandomInterval)
	{
		Init();
	}

	/** Base spawn interval in milliseconds */


	int BaseSpawnIntervalMs;

	/** Random spawn interval in milliseconds */
	int RandomSpawnIntervalMs;

	/** Next spawn time */
	Clock::time_point nextSpawn;

	/** Initialize spawner */
	void Init()
	{
		std::srand(static_cast<unsigned>(GetTickCount64()));
		auto ms = BaseSpawnIntervalMs + (std::rand() % RandomSpawnIntervalMs);
		nextSpawn = Clock::now() + std::chrono::milliseconds(ms);
	}

	/** Update spawner each tick */
	void Tick(FPrimitiveVector* PrimitiveVector)
	{
		auto now = Clock::now();
		if (now >= nextSpawn)
		{
			if (PrimitiveVector->size() < 100)
			{
				if (rand() % 5 < 4)
				{
					// ENEMY는 계속 움직이므로 겹쳐도 괜찮습니다.
					PrimitiveVector->push_back(new UEnemy());
				}
				else
				{
					// PREY는 겹치지 않도록 안전한 위치를 찾아 생성합니다.
					UPrey* newPrey = new UPrey();
					FVector safeLocation = PrimitiveVector->FindSafeSpawnLocation(newPrey->GetRadius());
					newPrey->SetLocation(safeLocation);
					PrimitiveVector->push_back(newPrey);
				}
			}

			// 타이머를 새로 설정합니다.
			Init();
		}
	}
};

// WinMain 함수가 시작되기 전에 이 함수를 추가하세요.


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	WCHAR WindowClass[] = L"JungleWindowClass";
	WCHAR Title[] = L"Game Tech Lab";
	WNDCLASSW wndClass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndClass);

	hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
						   CW_USEDEFAULT, CW_USEDEFAULT, winWidth, winHeight, nullptr, nullptr, hInstance, nullptr);

	// --- 초기화 ---
	srand((unsigned int)time(NULL));

	// Sound Initialize
	if (!USoundManager::Initialize())
	{
		//return 0;
	}
	if (!USoundManager::LoadAllSounds())
	{
		// 로드 실패해도 게임은 계속 진행 (사운드 없이)
	}
	USoundManager::BGM();

	URenderer renderer;
	renderer.Create(hWnd);
	///renderer.CreateShader();
	renderer.CreateUnitShader();
	renderer.CreateConstantBuffer();
	renderer.CreateUIResources();
	
	MenuUI menuUI;


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	FVertexSimple* verticesSphere = sphere_vertices;
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(verticesSphere, sizeof(sphere_vertices));

	FPrimitiveVector PrimitiveVector;
	UCamera* cam = new UCamera();
	UEnemySpawner enemySpawner(200, 100); // 0.4초 ~ 0.6초마다 적 생성을 위한 타이머
	UEnemySpawner preySpawner(1000, 500);  // 1.0초 ~ 1.5초마다 먹이 생성을 위한 타이머

	// --- 타이머 설정 ---
	LARGE_INTEGER frequency;
	LARGE_INTEGER CreateStartTime;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&CreateStartTime);
	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0;

	// --- 메인 루프 ---
	bool bIsExit = false;
	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);

		MSG msg;
		InputManager::Input().BeginFrame();
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}
		renderer.Prepare();
		renderer.PrepareUnitShader();
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		 

		//Test Time
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		double elapsed = double(now.QuadPart - CreateStartTime.QuadPart) / double(frequency.QuadPart);
		float iTime = static_cast<float>(elapsed);

		 
		////////// UI TEST //////////  
		switch (ScreenState)
		{
		case Screen::MainMenu:
		{
			MenuActions action = menuUI.DrawMainMenu(renderer, hWnd);
			if (action.start)
			{
				// ★★ 수정 2: 'Start' 버튼을 눌렀을 때만 객체를 생성합니다.

				// 만약을 위해 기존 객체들을 모두 삭제 (재시작 기능 대비)
				while (PrimitiveVector.size() > 0)
				{
					PrimitiveVector.RemoveAt(0);
				}
				PrimitiveVector.Player = nullptr;

				// 플레이어 생성 및 추가
				UPlayer* player = new UPlayer();
				PrimitiveVector.push_back(player);

				// 초기 ENEMY와 PREY 생성
				for (int i = 0; i < 30; ++i)
				{
					if (i % 2 == 0) {
						UPrey* newPrey = new UPrey();
						// 안전한 위치를 찾아서 설정
						FVector safeLocation = PrimitiveVector.FindSafeSpawnLocation(newPrey->GetRadius());
						newPrey->SetLocation(safeLocation);
						PrimitiveVector.push_back(newPrey);
					}
				}
				enemySpawner.Init(); // ★★ 추가: ENEMY 타이머 초기화
				preySpawner.Init();  // ★★ 추가: PREY 타이머 초기화
				ScreenState = Screen::Running; // 게임 상태를 '진행 중'으로 변경
			}
			if (action.gameover)
				ScreenState = Screen::EndingMenu;

			if (action.exit)
				bIsExit = true;
			break;
		}
		case Screen::Running:
		{
			auto now = std::chrono::steady_clock::now(); // 현재 시간 가져오기

			// ENEMY 타이머 확인 및 생성
			if (now >= enemySpawner.nextSpawn)
			{
				if (PrimitiveVector.size() < 100)
					PrimitiveVector.push_back(new UEnemy());
				enemySpawner.Init(); // ENEMY 타이머 리셋
			}

			// PREY 타이머 확인 및 생성
			if (now >= preySpawner.nextSpawn)
			{
				if (PrimitiveVector.size() < 100)
				{
					// 1. Prey를 만들고
					UPrey* newPrey = new UPrey();
					// 2. 안전한 위치를 찾아서
					FVector safeLocation = PrimitiveVector.FindSafeSpawnLocation(newPrey->GetRadius());
					// 3. 위치를 설정한 뒤에 게임에 추가합니다.
					newPrey->SetLocation(safeLocation);
					PrimitiveVector.push_back(newPrey);
				}
				preySpawner.Init(); // PREY 타이머 리셋
			}
			// --- 업데이트 로직 ---
			for (int i = 0; i < PrimitiveVector.size(); i++)
			{
				PrimitiveVector[i]->Movement();
			}
			PrimitiveVector.ProcessGameLogic();
			if (PrimitiveVector.Player)
			{
				//cam->UpdateCamera(PrimitiveVector.Player);
			}
			if (PrimitiveVector.Player && PrimitiveVector.Player->GetRadius() < 0.02f)
			{
				ScreenState = Screen::EndingMenu;
			}
			break;
		}
		case Screen::EndingMenu:
		{
			MenuActions action = menuUI.DrawEndingMenu(renderer, hWnd);
			if (action.start)
			{
				// 재시작 로직 (MainMenu와 동일)
				while (PrimitiveVector.size() > 0) { PrimitiveVector.RemoveAt(0); }
				PrimitiveVector.Player = nullptr;
				UPlayer* player = new UPlayer();
				PrimitiveVector.push_back(player);
				for (int i = 0; i < 20; ++i)
				{
					PrimitiveVector.push_back(new UEnemy());
					if (i % 2 == 0) PrimitiveVector.push_back(new UPrey());
				}
				enemySpawner.Init(); // ★★ 추가: ENEMY 타이머 초기화
				preySpawner.Init();  // ★★ 추가: PREY 타이머 초기화
				ScreenState = Screen::Running;
			}
			if (action.exit)
				bIsExit = true;
			break;
		}

		}
			// --- 렌더링 로직 ---
		renderer.PrepareShader();
		std::vector<int> visiblePrimitives, invisiblePrimitives;
		PrimitiveVector.ClassifyBorder(cam, visiblePrimitives, invisiblePrimitives);

		for (int idx : visiblePrimitives)
		{
			UPrimitive* prim = PrimitiveVector[idx];
			if (prim != nullptr)
			{
				FVector objectColor;
				switch (prim->GetAttribute())
				{
				case FIRE:  objectColor = FVector(1.0f, 0.2f, 0.2f); break;
				case WATER: objectColor = FVector(0.2f, 0.5f, 1.0f); break;
				case GRASS: objectColor = FVector(0.2f, 1.0f, 0.3f); break;
				}
				FVector renderedLocation = cam->ConvertToCameraSpaceLocation(prim->GetLocation());
				float renderedRadius = cam->ConvertToCameraSpaceRadius(prim->GetRadius());
				// renderer.UpdateConstant(renderedLocation, renderedRadius);
				renderer.UpdateUnitConstant(prim->GetAttribute(), iTime, renderedLocation, renderedRadius);
				renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
			}
		}
		PrimitiveVector.RemoveOutsidePrimitives(invisiblePrimitives);

		// --- 게임 UI (ImGui) ---
		ImGui::Begin("Game Info");
		ImGui::Text("Score: %d", PrimitiveVector.Player ? PrimitiveVector.Player->GetScore() : 0);
		ImGui::Text("Objects: %d", PrimitiveVector.size());
		if (PrimitiveVector.Player)
		{
			EAttribute attr = PrimitiveVector.Player->GetAttribute();
			const char* attrText = (attr == WATER) ? "WATER" : (attr == FIRE) ? "FIRE" : "GRASS";
			ImGui::Text("Player Attribute: %s", attrText);
		}
		ImGui::End();
		



		// --- 프레임 종료 ---
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Swap Buffer
		renderer.SwapBuffer();

		// --- 프레임 속도 조절 ---
		do
		{
			QueryPerformanceCounter(&endTime);
			elapsedTime = (double)(endTime.QuadPart - startTime.QuadPart);
		} while (elapsedTime / frequency.QuadPart < (1.0 / 60.0));
	}

	// --- 리소스 해제 ---
	delete cam; // new로 생성했으므로 delete 필요
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	renderer.ReleaseVertexBuffer(vertexBufferSphere);
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.ReleaseUIResource();
	renderer.Release();

	      

	return 0;
}