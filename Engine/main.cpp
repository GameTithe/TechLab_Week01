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
// Primitive
#include "Primitive.h"


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

	// 무게중심을 기준으로 플레이어 전체 군집을 감싸는 원의 최소 반지름
	float GetPlayerSwarmRadius()
	{
		FVector COM; // Center Of Mass
		if (!TryGetCenterOfMass(COM))
		{
			return 0.0f; // Default spawn radius
		}

		float MaxRadius = 0.0f;
		for (int i = 0; i < Count; ++i)
		{
			UPlayer* Cell = PlayerCells[i];
			if (!Cell)
			{
				continue;
			}
			float Dist = sqrt(FVector::Distance2(COM, Cell->GetLocation())) + Cell->GetRadius();
			MaxRadius = Dist > MaxRadius ? Dist : MaxRadius;
		}
		return MaxRadius;
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
	UPlayer* Player = nullptr;

	FPrimitiveVector()
	{
		Capacity = 10;
		Size = 0;

		primitives = new UPrimitive * [Capacity];
	}
	~FPrimitiveVector()
	{
		//for (int i = 0; i < Size; i++)
		//{
		//	delete primitives[i];
		//}
		//delete[] primitives;
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

				EAttribute playerAttr = Player->GetAttribute();
				EAttribute otherAttr = other->GetAttribute();
				if (otherAttr == EAttribute::NONE) /** 먹이인 경우*/
				{
					USoundManager::PreyEat();

					Player->AddScore(10);
					Player->SetRadius(Player->GetRadius() + 0.005f);

					RemoveAt(i); /** 상대 객체를 제거 */
				}
				else if (CheckWin(playerAttr, otherAttr)) /** 이기는 경우 */
				{
					USoundManager::Collide();

					float otherRadius = other->GetRadius();
					int scoreToAdd = static_cast<int>(otherRadius * otherRadius * 1000);
					Player->AddScore(scoreToAdd);

					float playerRadius = Player->GetRadius();
					float newRadius = sqrt(playerRadius * playerRadius + otherRadius * otherRadius);
					Player->SetRadius(newRadius);
					RemoveAt(i); /** 상대 객체를 제거 */
				}
				else if (playerAttr != otherAttr) /** 지는 경우 (비기지 않은 경우) */
				{
					USoundManager::PreyEat();

					Player->AddScore(-5);
					Player->SetRadius(Player->GetRadius() - 0.005f);
					RemoveAt(i); /** 상대 객체를 제거 */
				}

				else if (playerAttr == otherAttr) // 비기는 경우 (같은 속성)
				{
					// 충돌한 객체가 UEnemy 타입인지 확인합니다.
					UEnemy* enemy = dynamic_cast<UEnemy*>(other);
					if (enemy != nullptr)
					{
						if (Player->bIsKnockedBack == false) // 플레이어가 넉백 상태가 아닐 때만 아래 코드를 실행
						{
							// 1. 적(Enemy)은 아무런 영향을 받지 않습니다. (속도 변경 코드 없음)

							// 2. 플레이어가 튕겨나갈 방향(충돌 법선 벡터)을 계산합니다.
							FVector normal = Player->GetLocation() - other->GetLocation();
							float dist = sqrt(dist2);
							if (dist > 1e-6)
							{
								normal.x /= dist;
								normal.y /= dist;
							}

							// 3. 플레이어에게 튕겨나가는 속도를 설정합니다.
							const float bounceSpeed = 0.03f; // 튕겨나가는 힘 조절
							Player->SetVelocity(normal * bounceSpeed);
							Player->KnockedBackMaxSpeed = Player->GetVelocity().Magnitude(); // 넉백 상태에서의 최대 속도 설정

							// 4. 플레이어를 넉백 상태로 만들고, 시작 시간을 기록합니다.
							Player->bIsKnockedBack = true;
							Player->knockbackStartTime = std::chrono::steady_clock::now();

							// 5. 객체 겹침 문제를 해결합니다.
							float overlap = 0.5f * (minDist - dist);
							Player->SetLocation(Player->GetLocation() + normal * overlap);

							USoundManager::Collide();
						}
					}
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
				if (rand() % 5 < 3)
				{
					// ENEMY는 계속 움직이므로 겹쳐도 괜찮습니다.
					PrimitiveVector->push_back(new UEnemy());
				}
				else
				{
					// PREY는 겹치지 않도록 안전한 위치를 찾아 생성합니다.
					PrimitiveVector->push_back(new UPrey());
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
	UController Controller(10); // 최대 10개의 플레이어 셀을 제어할 수 있는 컨트롤러
	UCamera* Cam = new UCamera();
	UCamera::Main = Cam;
	UEnemySpawner enemySpawner(200, 100); // 0.4초 ~ 0.6초마다 적 생성을 위한 타이머

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

		//Test Time
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		double elapsed = double(now.QuadPart - CreateStartTime.QuadPart) / double(frequency.QuadPart);
		float iTime = static_cast<float>(elapsed);

		// Tick
		enemySpawner.Tick(&PrimitiveVector);
		
		// --- 렌더링 로직 ---
		renderer.PrepareUnitShader();
		std::vector<int> visiblePrimitives, invisiblePrimitives;
		PrimitiveVector.ClassifyBorder(Cam, visiblePrimitives, invisiblePrimitives);

		for (int idx : visiblePrimitives)
		{
			UPrimitive* prim = PrimitiveVector[idx];
			if (prim != nullptr)
			{
				FVector renderedLocation = Cam->ConvertToCameraSpaceLocation(prim->GetLocation());
				float renderedRadius = Cam->ConvertToCameraSpaceRadius(prim->GetRadius());
				// renderer.UpdateConstant(renderedLocation, renderedRadius);
				renderer.UpdateUnitConstant(prim->GetVelocity(), prim->GetAttribute(), iTime, renderedLocation, renderedRadius);
				renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
			}
		}
		PrimitiveVector.RemoveOutsidePrimitives(invisiblePrimitives);
		 

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
				Controller.Enroll(player); // 컨트롤러에 플레이어 셀 등록

				// 초기 ENEMY와 PREY 생성
				for (int i = 0; i < 30; ++i)
				{
					if (i % 2 == 0) {
						UPrey* newPrey = new UPrey();
						PrimitiveVector.push_back(newPrey);
					}
				}
				enemySpawner.Init(); // ★★ 추가: ENEMY 타이머 초기화
				RECT rect;
				GetClientRect(hWnd, &rect); // 윈도우의 클라이언트 영역 크기를 가져옴
				POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
				ClientToScreen(hWnd, &center); // 클라이언트 좌표를 화면 전체 좌표로 변환
				SetCursorPos(center.x, center.y); // 마우스 커서 위치를 설정
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
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			MenuActions action = menuUI.DrawRunningMenu(renderer, hWnd);
			auto now = std::chrono::steady_clock::now(); // 현재 시간 가져오기

			// ENEMY 타이머 확인 및 생성
			if (now >= enemySpawner.nextSpawn)
			{
				if (PrimitiveVector.size() < 100)
					PrimitiveVector.push_back(new UEnemy());
				enemySpawner.Init(); // ENEMY 타이머 리셋
			}

			if (Controller.Count > 0)
			{
				// 카메라 업데이트
				FVector CenterOfMass;
				if (Controller.TryGetCenterOfMass(CenterOfMass))
				{
					const float ReferenceRadius = 0.08f;
					float SwarmRadius = Controller.GetPlayerSwarmRadius();
					float DesiredScale = ReferenceRadius / SwarmRadius;
					Cam->UpdateCamera(CenterOfMass, DesiredScale);
				}
			}
			// 마우스 인풋 처리
			FVector CenterOfMass, CursorWorldLocation;
			if (Controller.TryGetCenterOfMass(CenterOfMass))
			{
				float normalizedX, normalizedY;
				InputManager::Input().GetNormalizedMousePos(normalizedX, normalizedY);
				CursorWorldLocation = UCamera::Main->ConvertToWorldSpaceLocation(FVector(normalizedX, normalizedY, 0.0f));
				for (int i = 0; i < Controller.Count; ++i)
				{
					UPlayer* cell = Controller.PlayerCells[i];
					if (cell)
					{
						cell->ApplyMouseForceAndGravity(CursorWorldLocation, CenterOfMass);
						// 현재 Movement() 자체적으로 마우스 인풋 처리 로직 가지고 있어 임시로 주석 처리.
						// Movement()를 통한 계산은 플레이어 분열시 확장성이 없어 추후 교체.
					}
				}
			}
			for (int i = 0; i < PrimitiveVector.size(); i++)
			{
				PrimitiveVector[i]->Movement();
			}
			PrimitiveVector.ProcessGameLogic();

			
			if (PrimitiveVector.Player && PrimitiveVector.Player->GetRadius() < 0.02f)
			{
				ScreenState = Screen::EndingMenu;
			}

			// --- 게임 UI (ImGui) ---
			ImGui::Begin("Game Info");			
			ImGui::Text("Camera Pos: %.2f %.2f %.2f ", Cam->Location.x, Cam->Location.y, Cam->Location.z);
			
			ImGuiIO& io = ImGui::GetIO();
			io.FontGlobalScale = 1.5f;
			ImGui::Text("Time: %.2f s", ImGui::GetTime());

			ImGui::Text("Score: %d", PrimitiveVector.Player ? PrimitiveVector.Player->GetScore() : 0);
			ImGui::Text("Objects: %d", PrimitiveVector.size());
			if (PrimitiveVector.Player)
			{
				EAttribute attr = PrimitiveVector.Player->GetAttribute();
				const char* attrText = (attr == WATER) ? "WATER" : (attr == FIRE) ? "FIRE" : "GRASS";
				ImGui::Text("Player Attribute: %s", attrText);
			}

			//시간제한
			if(ImGui::GetTime() >= 3.0f)
				ScreenState = Screen::EndingMenu;
			  
			ImGui::End();

			// --- 프레임 종료 ---
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


			if (action.menu)
			{
 				ScreenState = Screen::MainMenu;
			}
			if (action.exit)
				bIsExit = true;
			break;
		}
		case Screen::VictoryMenu:
		{
			MenuActions action = menuUI.DrawVictoryMenu(renderer, hWnd);
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
				enemySpawner.Init();
				ScreenState = Screen::Running;
			}
			if (action.exit)
				bIsExit = true;
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
				enemySpawner.Init();
				ScreenState = Screen::Running;
			}
			if (action.exit)
				bIsExit = true;
			break;
		}

		}
		

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
	delete Cam; // new로 생성했으므로 delete 필요
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