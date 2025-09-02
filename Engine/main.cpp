#define NOMINMAX
#include <windows.h>
#include <algorithm>

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
#include "UIInfo.h"
#include <iostream> // cout 사용을 위해
#include <ctime>    // time() 함수를 위해
#include <cstdlib>  // srand(), rand() 함수를 위해


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 각종 메세지를 처리할 함수
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	switch (message)
	{
	case WM_DESTROY:
		// signal the the app should quit
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

struct FVertexSimple
{
	float x, y, z;
	float r, g, b, a;
};

struct FVector
{
	float x, y, z;
	FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z)
	{

	}

	static float Distance2(const FVector& a, const FVector& b)
	{
		float dx = a.x - b.x;
		float dy = a.y - b.y;
		float dz = a.z - b.z;
		return (dx * dx + dy * dy + dz * dz);
	}
	float Magnitude()
	{
		return  sqrt(x * x + y * y + z * z);
	}
	void Normalize()
	{
		float len = sqrt(x * x + y * y + z * z);
		if (len > 1e-6f)
		{
			x /= len;
			y /= len;
			z /= len;
		}
		else
		{
			x = y = z = 0.0f;
		}
	}
	inline FVector& operator+=(const FVector& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;

		return *this;
	}

	inline FVector operator+(const FVector& vec) const
	{
		return FVector(x + vec.x, y + vec.y, z + vec.z);

	}

	inline FVector& operator-=(const FVector& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;

		return *this;
	}

	inline FVector operator-(const FVector& vec) const
	{
		return FVector(x - vec.x, y - vec.y, z - vec.z);

	}

	inline FVector& operator*=(const FVector& vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;

		return *this;
	}
	inline FVector operator*(const FVector& vec) const
	{
		return FVector(x * vec.x, y * vec.y, z * vec.z);
	}

	inline FVector operator*(float scalar) const
	{
		return FVector(x * scalar, y * scalar, z * scalar);
	}

	inline FVector& operator/=(const FVector& vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;

		return *this;
	}
	inline FVector operator/(const FVector& vec) const
	{
		float rtnX = (vec.x == 0.0f) ? 0.0f : x / vec.x;
		float rtnY = (vec.y == 0.0f) ? 0.0f : y / vec.y;
		float rtnZ = (vec.z == 0.0f) ? 0.0f : z / vec.z;

		return FVector(rtnX, rtnY, rtnZ);
	}

	inline FVector operator/(float scalar) const
	{
		if (scalar == 0)
		{
			return FVector(0.0f, 0.0f, 0.0f);
		}
		else
		{
			return FVector(x / scalar, y / scalar, z / scalar);
		}
	}
};

// 속성을 나타내기 위한 열거형(enum) 정의
enum EAttribute
{
	WATER, // 물
	FIRE,  // 불
	GRASS  // 풀
};

// 속성 상성을 체크하는 도우미 함수
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

class URenderer
{
public:
	// D3D11 Device, Device Context, Swap Chain을 관리하기 위한 포인터들 
	ID3D11Device* Device = nullptr;						//Gpu와 통신 하기 위한 Device
	ID3D11DeviceContext* DeviceContext = nullptr;		// GPU 명령 실행을 담당하는 Context
	IDXGISwapChain* SwapChain = nullptr;				// 프레인 버터를 교체하는 데 사용되는 Swap Chain

	// 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들 
	ID3D11Texture2D* FrameBuffer = nullptr;					// 화면 출력용
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;		// 텍스처를 렌터 타겟으로 사용하는 뷰
	ID3D11RasterizerState* RasterizerState = nullptr;		// Rasterizer 상태 (컬링, 채우기 모드 등) 
	ID3D11Buffer* ConstantBuffer = nullptr;					// Constant Buffer (셰이더에 전달할 데이터 저장용)


	//UI
	ID3D11VertexShader* UIVS = nullptr;
	ID3D11PixelShader* UIPS = nullptr;
	ID3D11InputLayout* UIInputLayout = nullptr;
	ID3D11Buffer* UIVertexBuffer = nullptr;  // 동적 VB
	ID3D11Buffer* UIPerFrameCB = nullptr;
	ID3D11ShaderResourceView* UISRV = nullptr;
	ID3D11SamplerState* UISampler = nullptr;
	ID3D11BlendState* UIAlphaBlend = nullptr;

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	D3D11_VIEWPORT ViewportInfo;	// 렌더링 영역을 정의하는 뷰포트 정보 

public:

	// 렌더러 초기화 함수
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);

		CreateFrameBuffer();

		CreateRasterizerState();

		// depth/stencil buffer & blend state는 다루지 않음 
	}


	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		// 지원하는 Direct3D 기능 레벨을 정의 
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		// Swap Chain 
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width = 0; //창 크기에 맞기 자동조정
		swapChainDesc.BufferDesc.Height = 0; //창 크기에 맞기 자동조정	
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 일반적인 RGBA 포맷
		swapChainDesc.SampleDesc.Count = 1; // 멀티 샘플링을 사용하지 않음
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
		swapChainDesc.BufferCount = 2; // 더블 버퍼링
		swapChainDesc.OutputWindow = hWindow;
		swapChainDesc.Windowed = TRUE; // 창 모드 
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

		// Create Dircet Deivce & Swap Chain
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &Device, nullptr, &DeviceContext);

		// SwapChain  정보 가져오기
		SwapChain->GetDesc(&swapChainDesc);

		// Set Viewport
		ViewportInfo = { 0.0f, 0.0f, (float)swapChainDesc.BufferDesc.Width, (float)swapChainDesc.BufferDesc.Height, 0.0f, 1.0f };
	}

	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext)
		{
			DeviceContext->Flush(); // 남아있는 GPU 명령을 모두 실행
		}

		if (SwapChain)
		{
			SwapChain->Release();
			SwapChain = nullptr;
		}

		if (Device)
		{
			DeviceContext->Release();
			DeviceContext = nullptr;
		}
	}

	void CreateFrameBuffer()
	{
		// 백 버처 텍스처 가져오기
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		// RTV 생성
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 일반적인 RGBA 포맷
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처로 설정	

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
	}

	void ReleaseFrameBuffer()
	{
		if (FrameBuffer)
		{
			FrameBuffer->Release();
			FrameBuffer = nullptr;
		}
		if (FrameBufferRTV)
		{
			FrameBufferRTV->Release();
			FrameBufferRTV = nullptr;
		}
	}

	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드  
		rasterizerDesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링  

		Device->CreateRasterizerState(&rasterizerDesc, &RasterizerState);
	}

	void ReleaseRasterizerState()
	{
		if (RasterizerState)
		{
			RasterizerState->Release();
			RasterizerState = nullptr;
		}
	}

	void Release()
	{
		RasterizerState->Release();

		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // 렌더 타겟을 초기화


		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0); // 1: VSync 활성화 => GPU 프레임 속도 & 화면 갱신 속도 동기화
	}

	void CreateUIResource()
	{
		//쉐이더 컴파일
	}

	void ReleaseUIResource()
	{

	}


	//Shader
public:
	ID3D11VertexShader* SimpleVertexShader;
	ID3D11PixelShader* SimplePixelShader;
	ID3D11InputLayout* SimpleInputLayout;
	unsigned int Stride;

	void CreateShader()
	{
		ID3DBlob* vertexShaderCSO;
		ID3DBlob* pixelShaderCSO;
		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexShaderCSO, nullptr);

		Device->CreateVertexShader(vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelShaderCSO, nullptr);
		Device->CreatePixelShader(pixelShaderCSO->GetBufferPointer(), pixelShaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		// vertesxShader의 입력 시그니처와 호환되는지 확인해야되니까
		// layout에서 vertexShaderCSO를 필요로함 
		Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

		Stride = sizeof(FVertexSimple);

		vertexShaderCSO->Release();
		pixelShaderCSO->Release();
	}

	void ReleaseShader()
	{
		if (SimpleInputLayout)
		{
			SimpleInputLayout->Release();
			SimpleInputLayout = nullptr;
		}

		if (SimplePixelShader)
		{
			SimplePixelShader->Release();
			SimplePixelShader = nullptr;
		}

		if (SimpleVertexShader)
		{
			SimpleVertexShader->Release();
			SimpleVertexShader = nullptr;
		}
	}

public:
	void Prepare()
	{
		DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//Rasterization State
		DeviceContext->RSSetViewports(1, &ViewportInfo);
		DeviceContext->RSSetState(RasterizerState);

		DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
		DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	void PrepareShader()
	{
		DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
		DeviceContext->IASetInputLayout(SimpleInputLayout);

		if (ConstantBuffer)
		{
			DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		}
	}

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		UINT offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
		DeviceContext->Draw(numVertices, 0);
	}

	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
	{

		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.ByteWidth = byteWidth;
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexBufferSRD = { vertices };

		ID3D11Buffer* vertexBuffer;
		Device->CreateBuffer(&vertexBufferDesc, &vertexBufferSRD, &vertexBuffer);

		return vertexBuffer;
	}

	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
	{
		vertexBuffer->Release();
	}

public:
	struct FConstant
	{
		FVector Offset;
		float Scale;
	};


	void CreateConstantBuffer()
	{
		// 16byte 단위로 압축해야됨 
		D3D11_BUFFER_DESC constantBufferDesc = {};
		constantBufferDesc.ByteWidth = (sizeof(FConstant) + 0xf) & 0xfffffff0; // 16배수 정렬 
		constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		Device->CreateBuffer(&constantBufferDesc, nullptr, &ConstantBuffer);

	}

	void ReleaseConstantBuffer()
	{
		if (ConstantBuffer)
		{
			ConstantBuffer->Release();
			ConstantBuffer = nullptr;
		}
	}

	void UpdateConstant(FVector Offset, float Scale)
	{
		if (ConstantBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE constantBufferMSR;

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantBufferMSR);
			FConstant* constants = (FConstant*)constantBufferMSR.pData;
			constants->Offset = Offset;
			constants->Scale = Scale;

			DeviceContext->Unmap(ConstantBuffer, 0);
		}
	}


};

class UCamera
{
public:
	FVector Location;
	float RenderScale = 1.0f;
	float TargetRenderScale = 1.0f;

	float RefRadius = 0.2f;    // RenderScale = 1.0 기준
	float MinScale = 0.15f;    // 스케일 하한선 (너무 작아져서 점처럼 보이지 않게)
	float MaxScale = 2.0f;     // 스케일 상한선 (과도한 줌인 방지)
	float SmoothT = 0.2f;      // Lerp 비율
public:
	void SetLocation(FVector location)
	{
		this->Location = location;
	}

	void UpdateCamera(UPrimitive* Player)
	{
		SetLocation(Player->GetLocation());

		// 실제 렌더링 스케일을 목표 스케일을 향해 점진적으로 조정
		if (RenderScale != TargetRenderScale)
		{
			float playerRadius = std::max(Player->GetRadius(), 0.001f);
			TargetRenderScale = RefRadius / playerRadius;
			TargetRenderScale = std::max(MinScale, std::min(MaxScale, TargetRenderScale));
			RenderScale = SmoothT * TargetRenderScale + (1.0f - SmoothT) * RenderScale;

			float t = 0.3f;
			RenderScale = t * TargetRenderScale + (1.0 - t) * RenderScale;
			float diff = RenderScale - TargetRenderScale;
			if (fabs(RenderScale - TargetRenderScale) < 0.01f)
			{
				RenderScale = TargetRenderScale;
			}
		}
	}

	FVector GetCameraSpaceLocation(UPrimitive* primitive)
	{
		return (primitive->GetLocation() - this->Location) * RenderScale;
	}

	float GetCameraSpaceRadius(UPrimitive* primitive)
	{
		return primitive->GetRadius() * RenderScale;
	}
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
	// 생성자: 플레이어 생성 시 호출됨
	UPlayer()
	{
		// 1. 플레이어 속성을 랜덤으로 선택
		Attribute = (EAttribute)(rand() % 3); // 0, 1, 2 중 하나를 랜덤으로 뽑아 속성으로 지정

		Radius = 0.08f; // 초기 크기
		Mass = Radius * 10.0f;
		Location = FVector(0.0f, 0.0f, 0.0f); // 화면 중앙에서 시작
		Velocity = FVector(0.0f, 0.0f, 0.0f); // 속도는 마우스를 따르므로 0으로 시작
		Score = 0;
	}

	// UPrimitive의 규칙(순수 가상 함수)에 따라 모든 함수를 구현
	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }

	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }

	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// 규칙에 따라 GetAttribute 함수를 구현
	virtual EAttribute GetAttribute() const override {return Attribute;} 
	// 사용되지 않는 기능들은 기본 형태로 구현
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// 플레이어의 핵심 로직: 마우스를 따라 움직임
	virtual void Movement() override
	{
		extern HWND hWnd; // WinMain의 hWnd를 외부에서 참조

		POINT mousePos;
		GetCursorPos(&mousePos); // 마우스의 스크린 좌표를 얻음
		ScreenToClient(hWnd, &mousePos); // 스크린 좌표를 프로그램 창 내부 좌표로 변환

		RECT clientRect;
		GetClientRect(hWnd, &clientRect); // 프로그램 창의 크기를 얻음

		// 창 내부 좌표(e.g., 0~1024)를 게임 월드 좌표(-1.0 ~ 1.0)로 변환
		float worldX = ((float)mousePos.x / clientRect.right) * 2.0f - 1.0f;
		float worldY = (-(float)mousePos.y / clientRect.bottom) * 2.0f + 1.0f;

		Location.x = worldX;
		Location.y = worldY;
	}

	// 크기와 점수를 조절하는 새로운 함수들
	void AddScore(int amount) { Score += amount; }
	int GetScore() const { return Score; }
	void SetRadius(float newRadius)
	{
		Radius = newRadius;
		// 크기가 변하면 질량도 같이 업데이트
		Mass = Radius * 10.0f;
	}


public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	int Score;
	EAttribute Attribute;
};

class UEnemy : public UPrimitive
{
public:
	// 생성자: ENEMY 생성 시 호출됨
	UEnemy()
	{
		// 무작위 속성, 위치, 속도, 크기 설정
		Attribute = (EAttribute)(rand() % 3);
		Location = FVector((rand() / (float)RAND_MAX) * 2.0f - 1.0f, (rand() / (float)RAND_MAX) * 2.0f - 1.0f, 0.0f);

		const float enemySpeed = 0.001f;
		Velocity.x = (float)(rand() % 100 - 50) * enemySpeed;
		Velocity.y = (float)(rand() % 100 - 50) * enemySpeed;

		Radius = ((rand() / (float)RAND_MAX)) * 0.1f + 0.03f; // 최소, 최대 크기 지정
		Mass = Radius * 10.0f;
	}

	// UPrimitive의 규칙에 따라 모든 함수를 구현

	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }
	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }
	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// 규칙에 따라 GetAttribute 함수를 구현
	virtual EAttribute GetAttribute() const override { return Attribute; }
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// ENEMY의 움직임: 기존 UBall처럼 벽에 튕김
	virtual void Movement() override
	{
		Location += Velocity;

		if (Location.x > 1.0f - Radius || Location.x < -1.0f + Radius)
		{
			Velocity.x *= -1.0f;
		}
		if (Location.y > 1.0f - Radius || Location.y < -1.0f + Radius)
		{
			Velocity.y *= -1.0f;
		}
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
	// 생성자: PREY 생성 시 호출됨
	UPrey()
	{
		// 무작위 속성, 위치, 크기 설정
		Attribute = (EAttribute)(rand() % 3);
		Location = FVector((rand() / (float)RAND_MAX) * 2.0f - 1.0f, (rand() / (float)RAND_MAX) * 2.0f - 1.0f, 0.0f);
		Velocity = FVector(0.0f, 0.0f, 0.0f); // 움직이지 않으므로 속도는 0
		Radius = ((rand() / (float)RAND_MAX)) * 0.05f + 0.02f;
		Mass = Radius * 10.0f;
	}

	// UPrimitive의 규칙에 따라 모든 함수를 구현
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

	// PREY의 움직임: 움직이지 않음
	virtual void Movement() override
	{
		// 아무 코드도 없음
	}

public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
};

//int UBall::TotalBalls = 0;
//ID3D11Buffer* UBall::vertexBufferSphere = nullptr; // static 멤버 변수 초기화

struct Merge
{
	int indexA;
	int indexB;
};

class FPrimitiveVector
{
public:
	// 플레이어 객체에 쉽게 접근하기 위한 포인터
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

		// 만약 추가되는 객체가 플레이어라면, Player 포인터에 저장
		// dynamic_cast는 UPrimitive*를 UPlayer*로 안전하게 변환 시도
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
		// 유효하지 않은 인덱스면 즉시 종료
		if (index < 0 || index >= Size)
		{
			return;
		}

		// 객체 메모리 해제
		delete primitives[index];

		// 마지막 원소를 현재 위치로 이동 (가장 빠른 제거 방법)
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

	int size() const // 캡슐화를 대비해서 함수로 접근
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
		if (Player == nullptr) return; // 플레이어가 없으면 아무것도 안 함

		// 배열을 거꾸로 순회해야 객체 제거 시에도 안전함
		for (int i = Size - 1; i >= 0; --i)
		{
			// 자기 자신(플레이어)과는 충돌 검사를 하지 않음
			if (primitives[i] == Player)
			{
				continue;
			}

			UPrimitive* other = primitives[i];
			float dist2 = FVector::Distance2(Player->GetLocation(), other->GetLocation());
			float minDist = Player->GetRadius() + other->GetRadius();

			// 충돌했다면
			if (dist2 < minDist * minDist)
			{
				// 이기는 상성인지 체크
				if (CheckWin(Player->GetAttribute(), other->GetAttribute()))
				{
					Player->AddScore(10);
					Player->SetRadius(Player->GetRadius() + 0.005f); // 크기 증가
				}
				else // 지거나 비기는 상성
				{
					Player->AddScore(-5);
					Player->SetRadius(Player->GetRadius() - 0.005f); // 크기 감소
				}

				// 충돌한 객체는 제거
				RemoveAt(i);
			}
		}
	}
	//void CollisionCheck(float elastic, bool bCombination)
	//{
	//	for (int i = 0; i < Size; ++i)
	//	{
	//		for (int j = i + 1; j < Size; ++j)
	//		{
	//			UPrimitive* a = primitives[i];
	//			UPrimitive* b = primitives[j];

	//			float radius1 = a->GetRadius();
	//			float radius2 = b->GetRadius();

	//			FVector pos1 = a->GetLocation();
	//			FVector pos2 = b->GetLocation();

	//			float dist2 = FVector::Distance2(pos2, pos1);
	//			float minDist = radius1 + radius2;

	//			// 구와 구의 충돌처리 sqrt 비용이 비싸기 때문에 squre되어있는 상태에서 거리 비교
	//			if (dist2 < minDist * minDist)
	//			{

	//				if (bCombination && mergeCount < 1024)
	//				{
	//					mergeList[mergeCount] = { i, j };
	//					mergeCount++;
	//				}
	//				//Combine이 아니거나 mergeCount가 최대 merge보다 클 때
	//				else
	//				{
	//					float dist = sqrt(dist2);
	//					FVector normal = (pos2 - pos1);
	//					normal.Normalize();

	//					FVector velocityOfA = a->GetVelocity();
	//					FVector velocityOfB = b->GetVelocity();

	//					FVector relativeVelocity = velocityOfB - velocityOfA;
	//					float speed = Dot(relativeVelocity, normal);

	//					// 상대 속도와 충돌된 구와의 방향이 같을 때만 충격량 계산
	//					if (speed < 0.0f)
	//					{
	//						float massOfA = a->GetMass();
	//						float massOfB = b->GetMass();

	//						float impulse = -(1 + elastic) * speed / (1 / massOfA + 1 / massOfB);
	//						FVector J = normal * impulse;

	//						a->SetVelocity(velocityOfA - J / massOfA);
	//						b->SetVelocity(velocityOfB + J / massOfB);
	//					}

	//					// 위치 보정
	//					float penetration = minDist - dist;
	//					FVector correction = normal * (penetration * 0.5f);
	//					a->SetLocation(pos1 - correction);
	//					b->SetLocation(pos2 + correction);
	//				}
	//			}
	//		}
	//	}
	//}
	  
	// 쿨룽 식으로 자기력 효과 
	// 거리 제곱에 반비례
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

				float k = 0.001f; //수치 안정을 위해서 임의로 조정
				float q1 = a->GetMagnetic();
				float q2 = b->GetMagnetic();
				float dist2 = FVector::Distance2(a->GetLocation(), b->GetLocation());

				dist2 = dist2 == 0 ? 1e-5 : dist2;
				float force = q1 * q2 * k / dist2;

				FVector normal = a->GetLocation() - b->GetLocation();
				//normal.Normalize(); // normalize해주면 너무 커질 수 있으니 패스

				totalMagneticForce += normal * force;
			}
			FVector acceleration = totalMagneticForce / a->GetMass();

			a->SetVelocity(a->GetVelocity() + acceleration);
		}
	}

	////일정 반지름 이하일 때 분할하도록 유도
	//void Explosion()
	//{
	//	for (int i = 0; i < Size; ++i)
	//	{
	//		UPrimitive* primitive = primitives[i];
	//		if (primitive->GetRadius() > 0.05f)
	//		{
	//			primitive->SetDivide(true);
	//		}
	//	}

	//	for (int i = 0; i < Size; ++i)
	//	{
	//		UPrimitive* primitive = primitives[i];
	//		if (primitive->GetDivide())
	//		{
	//			FVector location = primitive->GetLocation();
	//			FVector velocity = primitive->GetVelocity();
	//			float radius = primitive->GetRadius() * 0.5f;

	//			//새로운 공 생성
	//			UPrimitive* newBall1 = new UBall(radius);
	//			newBall1->SetLocation(FVector(location.x + radius, location.y, location.z));
	//			newBall1->SetVelocity(FVector(velocity.x + 0.01f, velocity.y, velocity.z));
	//			push_back(newBall1);

	//			UPrimitive* newBall2 = new UBall(radius);
	//			newBall2->SetLocation(FVector(location.x - radius, location.y, location.z));
	//			newBall2->SetVelocity(FVector(velocity.x - 0.01f, velocity.y, velocity.z));
	//			push_back(newBall2);

	//			//기존 공 제거
	//			delete primitive;
	//			primitives[i] = nullptr;
	//			primitives[i] = primitives[Size - 1];
	//			primitives[i]->SetDivide(false);
	//			Size--;
	//		}
	//	}
	//}

	//void Combination()
	//{
	//	//큰 index먼저 처리를 위해서 정렬
	//	for (int i = 0; i < mergeCount - 1; ++i)
	//	{
	//		for (int j = i + 1; j < mergeCount; ++j)
	//		{
	//			int maxI = mergeList[i].indexA > mergeList[i].indexB ? mergeList[i].indexA : mergeList[i].indexB;
	//			int maxJ = mergeList[j].indexA > mergeList[j].indexB ? mergeList[j].indexA : mergeList[j].indexB;

	//			if (maxI < maxJ) // 내림차순 정렬
	//			{
	//				Merge temp = mergeList[i];
	//				mergeList[i] = mergeList[j];
	//				mergeList[j] = temp;
	//			}
	//		}

	//	}
	//	for (int i = 0; i < mergeCount; i++)
	//	{
	//		int indexA = mergeList[i].indexA;
	//		int indexB = mergeList[i].indexB;

	//		//안에 있는 index 중에서도 큰 값먼저 계산하기 위함
	//		if (indexA < indexB)
	//		{
	//			int temp = indexA;
	//			indexA = indexB;
	//			indexB = temp;
	//		}

	//		//예외처리 
	//		if (indexA == -1 || indexB == -1)
	//		{
	//			continue;
	//		}
	//		if (indexA >= Size || indexB >= Size)
	//		{
	//			continue;
	//		}
	//		UPrimitive* a = primitives[indexA];
	//		UPrimitive* b = primitives[indexB];

	//		FVector newLocation = (a->GetLocation() + a->GetLocation()) * 0.5f;
	//		float radiusA = a->GetRadius();
	//		float radiusB = b->GetRadius();


	//		delete a;
	//		primitives[indexA] = primitives[Size - 1];
	//		Size--;

	//		delete b;
	//		primitives[indexB] = primitives[Size - 1];
	//		Size--;

	//		float newRadius = radiusA + radiusB;
	//		newRadius = newRadius > 1.0f ? 1.0f : newRadius;
	//		UBall* newBall = new UBall(newRadius);
	//		newBall->SetLocation(newLocation);
	//		push_back(newBall);
	//	}

	//	//clear
	//	for (int i = 0; i < mergeCount; i++)
	//	{
	//		mergeList[i].indexA = -1;
	//		mergeList[i].indexB = -1;
	//	}
	//	mergeCount = 0;
	//}

public:

	UPrimitive** primitives = nullptr;
	int Capacity = 0;
	int Size = 0;

	Merge mergeList[1024] = { -1, -1 };// 1충돌당 2개의 구가 필요하다고 하면, 1024면 넉넉할 것으로 예상, 만약 1024번을 넘기면 그냥 충돌로 처리
	int mergeCount = 0;
};

//int DesireNumberOfBalls = UBall::TotalBalls;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메세지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 등록
	WNDCLASSW wndClass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// 윈도우 클래스 등록
	RegisterClassW(&wndClass);

	// 1024 * 1024 크기의 윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024, nullptr, nullptr, hInstance, nullptr);

	srand((unsigned int)time(NULL));

	//각종 생성/초기화 코드를 여기에 추가
	URenderer renderer;
	renderer.Create(hWnd);
	renderer.CreateShader();
	renderer.CreateConstantBuffer();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	FVertexSimple* verticesSphere = sphere_vertices;
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(verticesSphere, sizeof(sphere_vertices));
	
	FPrimitiveVector PrimitiveVector;

	// 플레이어 생성 및 추가
	UPlayer* player = new UPlayer();
	PrimitiveVector.push_back(player);

	// 초기 ENEMY와 PREY 생성
	for (int i = 0; i < 10; ++i)
	{
		PrimitiveVector.push_back(new UEnemy());
		PrimitiveVector.push_back(new UPrey());
	}

	// 구에 관한 Vertex Buffer는 한 번만 생성 후 재사용
	UBall::vertexBufferSphere = renderer.CreateVertexBuffer(verticesSphere, sizeof(sphere_vertices));

	bool bIsExit = false;
	
	bool bMagnetic = false;

	bool bExplosion = false;
	bool bCombination = false;


	float elastic = 1.f; 
	FVector windForce(0.0f, 0.0f, 0.0f);
	const int targetFPS = 30;
	const float deltaTime = 1.0 / targetFPS;
	const double targetFrameTime = 1000.0f / targetFPS; //목표 프레임

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0f;

	//// 공 하나 추가하고 시작
	//FPrimitiveVector PrimitiveVector;
	//UBall* ball = new UBall();
	//PrimitiveVector.push_back(ball);
	UCamera* cam = new UCamera();

	// Main Loop 
	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);

		MSG msg;

		//메시지 큐에서 msg를 꺼내오고 큐에서 제거함 
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

		//basic movement
		for (int i = 0; i < PrimitiveVector.size(); i++)
		{
			PrimitiveVector[i]->Movement();
		}

		////collision check
		//PrimitiveVector.CollisionCheck(elastic, bCombination);
		// 
		////Magnetic
		//if (bMagnetic)
		//{
		//	PrimitiveVector.MagneticForce();
		//}
		//if (bExplosion)
		//{
		//	PrimitiveVector.Explosion();
		//	bExplosion = false;
		//}

		//DesireNumberOfBalls = PrimitiveVector.size(); // 업데이트 후 공의 개수 최신화

		// Frame Update
		renderer.Prepare();
		renderer.PrepareShader();

		for (int i = 0; i < PrimitiveVector.size(); i++)
		{
			// 스크린 상에서의 좌표와 크기 계산
			UPrimitive* prim = PrimitiveVector[i];
			FVector renderedLocation = cam->GetCameraSpaceLocation(PrimitiveVector[i]);
			float renderedRadius = cam->GetCameraSpaceRadius(PrimitiveVector[i]);
			renderer.UpdateConstant(renderedLocation, renderedRadius);
			renderer.RenderPrimitive(UBall::vertexBufferSphere, numVerticesSphere);
		}

		// ImGui Update
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render() 사이에
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");

		    
		ImGui::Checkbox("Manentic ", &bMagnetic);

		if (ImGui::Button("Explore"))
		{
			bExplosion = !bExplosion;
		}

		ImGui::Checkbox("Combine", &bCombination);


		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Swap Buffer
		renderer.SwapBuffer();
		do
		{
			Sleep(0);

			QueryPerformanceCounter(&endTime);

			elapsedTime = (endTime.QuadPart - startTime.QuadPart);


		} while (elapsedTime < targetFrameTime);
	}

	// 여기에서 ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//// 소멸에 필요한 코드 
	//renderer.ReleaseVertexBuffer(UBall::vertexBufferSphere);
	
	// WinMain에서 생성한 버퍼 해제
	renderer.ReleaseVertexBuffer(vertexBufferSphere); 
	renderer.ReleaseConstantBuffer();

	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();


	return 0;
}