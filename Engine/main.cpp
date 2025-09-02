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
#include <iostream> // cout ����� ����
#include <ctime>    // time() �Լ��� ����
#include <cstdlib>  // srand(), rand() �Լ��� ����


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ���� �޼����� ó���� �Լ�
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

// �Ӽ��� ��Ÿ���� ���� ������(enum) ����
enum EAttribute
{
	WATER, // ��
	FIRE,  // ��
	GRASS  // Ǯ
};

// �Ӽ� ���� üũ�ϴ� ����� �Լ�
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
	// D3D11 Device, Device Context, Swap Chain�� �����ϱ� ���� �����͵� 
	ID3D11Device* Device = nullptr;						//Gpu�� ��� �ϱ� ���� Device
	ID3D11DeviceContext* DeviceContext = nullptr;		// GPU ��� ������ ����ϴ� Context
	IDXGISwapChain* SwapChain = nullptr;				// ������ ���͸� ��ü�ϴ� �� ���Ǵ� Swap Chain

	// �������� �ʿ��� ���ҽ� �� ���¸� �����ϱ� ���� ������ 
	ID3D11Texture2D* FrameBuffer = nullptr;					// ȭ�� ��¿�
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;		// �ؽ�ó�� ���� Ÿ������ ����ϴ� ��
	ID3D11RasterizerState* RasterizerState = nullptr;		// Rasterizer ���� (�ø�, ä��� ��� ��) 
	ID3D11Buffer* ConstantBuffer = nullptr;					// Constant Buffer (���̴��� ������ ������ �����)


	//UI
	ID3D11VertexShader* UIVS = nullptr;
	ID3D11PixelShader* UIPS = nullptr;
	ID3D11InputLayout* UIInputLayout = nullptr;
	ID3D11Buffer* UIVertexBuffer = nullptr;  // ���� VB
	ID3D11Buffer* UIPerFrameCB = nullptr;
	ID3D11ShaderResourceView* UISRV = nullptr;
	ID3D11SamplerState* UISampler = nullptr;
	ID3D11BlendState* UIAlphaBlend = nullptr;

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	D3D11_VIEWPORT ViewportInfo;	// ������ ������ �����ϴ� ����Ʈ ���� 

public:

	// ������ �ʱ�ȭ �Լ�
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);

		CreateFrameBuffer();

		CreateRasterizerState();

		// depth/stencil buffer & blend state�� �ٷ��� ���� 
	}


	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		// �����ϴ� Direct3D ��� ������ ���� 
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		// Swap Chain 
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width = 0; //â ũ�⿡ �±� �ڵ�����
		swapChainDesc.BufferDesc.Height = 0; //â ũ�⿡ �±� �ڵ�����	
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // �Ϲ����� RGBA ����
		swapChainDesc.SampleDesc.Count = 1; // ��Ƽ ���ø��� ������� ����
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ���� Ÿ������ ���
		swapChainDesc.BufferCount = 2; // ���� ���۸�
		swapChainDesc.OutputWindow = hWindow;
		swapChainDesc.Windowed = TRUE; // â ��� 
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ���� ���

		// Create Dircet Deivce & Swap Chain
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &Device, nullptr, &DeviceContext);

		// SwapChain  ���� ��������
		SwapChain->GetDesc(&swapChainDesc);

		// Set Viewport
		ViewportInfo = { 0.0f, 0.0f, (float)swapChainDesc.BufferDesc.Width, (float)swapChainDesc.BufferDesc.Height, 0.0f, 1.0f };
	}

	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext)
		{
			DeviceContext->Flush(); // �����ִ� GPU ����� ��� ����
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
		// �� ��ó �ؽ�ó ��������
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		// RTV ����
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // �Ϲ����� RGBA ����
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D �ؽ�ó�� ����	

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
		rasterizerDesc.FillMode = D3D11_FILL_SOLID; // ä��� ���  
		rasterizerDesc.CullMode = D3D11_CULL_BACK; // �� ���̽� �ø�  

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

		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // ���� Ÿ���� �ʱ�ȭ


		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0); // 1: VSync Ȱ��ȭ => GPU ������ �ӵ� & ȭ�� ���� �ӵ� ����ȭ
	}

	void CreateUIResource()
	{
		//���̴� ������
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

		// vertesxShader�� �Է� �ñ״�ó�� ȣȯ�Ǵ��� Ȯ���ؾߵǴϱ�
		// layout���� vertexShaderCSO�� �ʿ���� 
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
		// 16byte ������ �����ؾߵ� 
		D3D11_BUFFER_DESC constantBufferDesc = {};
		constantBufferDesc.ByteWidth = (sizeof(FConstant) + 0xf) & 0xfffffff0; // 16��� ���� 
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

	float RefRadius = 0.2f;    // RenderScale = 1.0 ����
	float MinScale = 0.15f;    // ������ ���Ѽ� (�ʹ� �۾����� ��ó�� ������ �ʰ�)
	float MaxScale = 2.0f;     // ������ ���Ѽ� (������ ���� ����)
	float SmoothT = 0.2f;      // Lerp ����
public:
	void SetLocation(FVector location)
	{
		this->Location = location;
	}

	void UpdateCamera(UPrimitive* Player)
	{
		SetLocation(Player->GetLocation());

		// ���� ������ �������� ��ǥ �������� ���� ���������� ����
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
	// ������: �÷��̾� ���� �� ȣ���
	UPlayer()
	{
		// 1. �÷��̾� �Ӽ��� �������� ����
		Attribute = (EAttribute)(rand() % 3); // 0, 1, 2 �� �ϳ��� �������� �̾� �Ӽ����� ����

		Radius = 0.08f; // �ʱ� ũ��
		Mass = Radius * 10.0f;
		Location = FVector(0.0f, 0.0f, 0.0f); // ȭ�� �߾ӿ��� ����
		Velocity = FVector(0.0f, 0.0f, 0.0f); // �ӵ��� ���콺�� �����Ƿ� 0���� ����
		Score = 0;
	}

	// UPrimitive�� ��Ģ(���� ���� �Լ�)�� ���� ��� �Լ��� ����
	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }

	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }

	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// ��Ģ�� ���� GetAttribute �Լ��� ����
	virtual EAttribute GetAttribute() const override {return Attribute;} 
	// ������ �ʴ� ��ɵ��� �⺻ ���·� ����
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// �÷��̾��� �ٽ� ����: ���콺�� ���� ������
	virtual void Movement() override
	{
		extern HWND hWnd; // WinMain�� hWnd�� �ܺο��� ����

		POINT mousePos;
		GetCursorPos(&mousePos); // ���콺�� ��ũ�� ��ǥ�� ����
		ScreenToClient(hWnd, &mousePos); // ��ũ�� ��ǥ�� ���α׷� â ���� ��ǥ�� ��ȯ

		RECT clientRect;
		GetClientRect(hWnd, &clientRect); // ���α׷� â�� ũ�⸦ ����

		// â ���� ��ǥ(e.g., 0~1024)�� ���� ���� ��ǥ(-1.0 ~ 1.0)�� ��ȯ
		float worldX = ((float)mousePos.x / clientRect.right) * 2.0f - 1.0f;
		float worldY = (-(float)mousePos.y / clientRect.bottom) * 2.0f + 1.0f;

		Location.x = worldX;
		Location.y = worldY;
	}

	// ũ��� ������ �����ϴ� ���ο� �Լ���
	void AddScore(int amount) { Score += amount; }
	int GetScore() const { return Score; }
	void SetRadius(float newRadius)
	{
		Radius = newRadius;
		// ũ�Ⱑ ���ϸ� ������ ���� ������Ʈ
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
	// ������: ENEMY ���� �� ȣ���
	UEnemy()
	{
		// ������ �Ӽ�, ��ġ, �ӵ�, ũ�� ����
		Attribute = (EAttribute)(rand() % 3);
		Location = FVector((rand() / (float)RAND_MAX) * 2.0f - 1.0f, (rand() / (float)RAND_MAX) * 2.0f - 1.0f, 0.0f);

		const float enemySpeed = 0.001f;
		Velocity.x = (float)(rand() % 100 - 50) * enemySpeed;
		Velocity.y = (float)(rand() % 100 - 50) * enemySpeed;

		Radius = ((rand() / (float)RAND_MAX)) * 0.1f + 0.03f; // �ּ�, �ִ� ũ�� ����
		Mass = Radius * 10.0f;
	}

	// UPrimitive�� ��Ģ�� ���� ��� �Լ��� ����

	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }
	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }
	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// ��Ģ�� ���� GetAttribute �Լ��� ����
	virtual EAttribute GetAttribute() const override { return Attribute; }
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// ENEMY�� ������: ���� UBalló�� ���� ƨ��
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
	// ������: PREY ���� �� ȣ���
	UPrey()
	{
		// ������ �Ӽ�, ��ġ, ũ�� ����
		Attribute = (EAttribute)(rand() % 3);
		Location = FVector((rand() / (float)RAND_MAX) * 2.0f - 1.0f, (rand() / (float)RAND_MAX) * 2.0f - 1.0f, 0.0f);
		Velocity = FVector(0.0f, 0.0f, 0.0f); // �������� �����Ƿ� �ӵ��� 0
		Radius = ((rand() / (float)RAND_MAX)) * 0.05f + 0.02f;
		Mass = Radius * 10.0f;
	}

	// UPrimitive�� ��Ģ�� ���� ��� �Լ��� ����
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

	// PREY�� ������: �������� ����
	virtual void Movement() override
	{
		// �ƹ� �ڵ嵵 ����
	}

public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
};

//int UBall::TotalBalls = 0;
//ID3D11Buffer* UBall::vertexBufferSphere = nullptr; // static ��� ���� �ʱ�ȭ

struct Merge
{
	int indexA;
	int indexB;
};

class FPrimitiveVector
{
public:
	// �÷��̾� ��ü�� ���� �����ϱ� ���� ������
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

		// ���� �߰��Ǵ� ��ü�� �÷��̾���, Player �����Ϳ� ����
		// dynamic_cast�� UPrimitive*�� UPlayer*�� �����ϰ� ��ȯ �õ�
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
		// ��ȿ���� ���� �ε����� ��� ����
		if (index < 0 || index >= Size)
		{
			return;
		}

		// ��ü �޸� ����
		delete primitives[index];

		// ������ ���Ҹ� ���� ��ġ�� �̵� (���� ���� ���� ���)
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

	int size() const // ĸ��ȭ�� ����ؼ� �Լ��� ����
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
		if (Player == nullptr) return; // �÷��̾ ������ �ƹ��͵� �� ��

		// �迭�� �Ųٷ� ��ȸ�ؾ� ��ü ���� �ÿ��� ������
		for (int i = Size - 1; i >= 0; --i)
		{
			// �ڱ� �ڽ�(�÷��̾�)���� �浹 �˻縦 ���� ����
			if (primitives[i] == Player)
			{
				continue;
			}

			UPrimitive* other = primitives[i];
			float dist2 = FVector::Distance2(Player->GetLocation(), other->GetLocation());
			float minDist = Player->GetRadius() + other->GetRadius();

			// �浹�ߴٸ�
			if (dist2 < minDist * minDist)
			{
				// �̱�� ������ üũ
				if (CheckWin(Player->GetAttribute(), other->GetAttribute()))
				{
					Player->AddScore(10);
					Player->SetRadius(Player->GetRadius() + 0.005f); // ũ�� ����
				}
				else // ���ų� ���� ��
				{
					Player->AddScore(-5);
					Player->SetRadius(Player->GetRadius() - 0.005f); // ũ�� ����
				}

				// �浹�� ��ü�� ����
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

	//			// ���� ���� �浹ó�� sqrt ����� ��α� ������ squre�Ǿ��ִ� ���¿��� �Ÿ� ��
	//			if (dist2 < minDist * minDist)
	//			{

	//				if (bCombination && mergeCount < 1024)
	//				{
	//					mergeList[mergeCount] = { i, j };
	//					mergeCount++;
	//				}
	//				//Combine�� �ƴϰų� mergeCount�� �ִ� merge���� Ŭ ��
	//				else
	//				{
	//					float dist = sqrt(dist2);
	//					FVector normal = (pos2 - pos1);
	//					normal.Normalize();

	//					FVector velocityOfA = a->GetVelocity();
	//					FVector velocityOfB = b->GetVelocity();

	//					FVector relativeVelocity = velocityOfB - velocityOfA;
	//					float speed = Dot(relativeVelocity, normal);

	//					// ��� �ӵ��� �浹�� ������ ������ ���� ���� ��ݷ� ���
	//					if (speed < 0.0f)
	//					{
	//						float massOfA = a->GetMass();
	//						float massOfB = b->GetMass();

	//						float impulse = -(1 + elastic) * speed / (1 / massOfA + 1 / massOfB);
	//						FVector J = normal * impulse;

	//						a->SetVelocity(velocityOfA - J / massOfA);
	//						b->SetVelocity(velocityOfB + J / massOfB);
	//					}

	//					// ��ġ ����
	//					float penetration = minDist - dist;
	//					FVector correction = normal * (penetration * 0.5f);
	//					a->SetLocation(pos1 - correction);
	//					b->SetLocation(pos2 + correction);
	//				}
	//			}
	//		}
	//	}
	//}
	  
	// ��� ������ �ڱ�� ȿ�� 
	// �Ÿ� ������ �ݺ��
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

				float k = 0.001f; //��ġ ������ ���ؼ� ���Ƿ� ����
				float q1 = a->GetMagnetic();
				float q2 = b->GetMagnetic();
				float dist2 = FVector::Distance2(a->GetLocation(), b->GetLocation());

				dist2 = dist2 == 0 ? 1e-5 : dist2;
				float force = q1 * q2 * k / dist2;

				FVector normal = a->GetLocation() - b->GetLocation();
				//normal.Normalize(); // normalize���ָ� �ʹ� Ŀ�� �� ������ �н�

				totalMagneticForce += normal * force;
			}
			FVector acceleration = totalMagneticForce / a->GetMass();

			a->SetVelocity(a->GetVelocity() + acceleration);
		}
	}

	////���� ������ ������ �� �����ϵ��� ����
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

	//			//���ο� �� ����
	//			UPrimitive* newBall1 = new UBall(radius);
	//			newBall1->SetLocation(FVector(location.x + radius, location.y, location.z));
	//			newBall1->SetVelocity(FVector(velocity.x + 0.01f, velocity.y, velocity.z));
	//			push_back(newBall1);

	//			UPrimitive* newBall2 = new UBall(radius);
	//			newBall2->SetLocation(FVector(location.x - radius, location.y, location.z));
	//			newBall2->SetVelocity(FVector(velocity.x - 0.01f, velocity.y, velocity.z));
	//			push_back(newBall2);

	//			//���� �� ����
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
	//	//ū index���� ó���� ���ؼ� ����
	//	for (int i = 0; i < mergeCount - 1; ++i)
	//	{
	//		for (int j = i + 1; j < mergeCount; ++j)
	//		{
	//			int maxI = mergeList[i].indexA > mergeList[i].indexB ? mergeList[i].indexA : mergeList[i].indexB;
	//			int maxJ = mergeList[j].indexA > mergeList[j].indexB ? mergeList[j].indexA : mergeList[j].indexB;

	//			if (maxI < maxJ) // �������� ����
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

	//		//�ȿ� �ִ� index �߿����� ū ������ ����ϱ� ����
	//		if (indexA < indexB)
	//		{
	//			int temp = indexA;
	//			indexA = indexB;
	//			indexB = temp;
	//		}

	//		//����ó�� 
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

	Merge mergeList[1024] = { -1, -1 };// 1�浹�� 2���� ���� �ʿ��ϴٰ� �ϸ�, 1024�� �˳��� ������ ����, ���� 1024���� �ѱ�� �׳� �浹�� ó��
	int mergeCount = 0;
};

//int DesireNumberOfBalls = UBall::TotalBalls;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// ������ Ŭ���� �̸�
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ������ Ÿ��Ʋ�� �̸�
	WCHAR Title[] = L"Game Tech Lab";

	// ���� �޼����� ó���� �Լ��� WndProc�� �Լ� �����͸� WindowClass ����ü�� ���
	WNDCLASSW wndClass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// ������ Ŭ���� ���
	RegisterClassW(&wndClass);

	// 1024 * 1024 ũ���� ������ ����
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024, nullptr, nullptr, hInstance, nullptr);

	srand((unsigned int)time(NULL));

	//���� ����/�ʱ�ȭ �ڵ带 ���⿡ �߰�
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

	// �÷��̾� ���� �� �߰�
	UPlayer* player = new UPlayer();
	PrimitiveVector.push_back(player);

	// �ʱ� ENEMY�� PREY ����
	for (int i = 0; i < 10; ++i)
	{
		PrimitiveVector.push_back(new UEnemy());
		PrimitiveVector.push_back(new UPrey());
	}

	// ���� ���� Vertex Buffer�� �� ���� ���� �� ����
	UBall::vertexBufferSphere = renderer.CreateVertexBuffer(verticesSphere, sizeof(sphere_vertices));

	bool bIsExit = false;
	
	bool bMagnetic = false;

	bool bExplosion = false;
	bool bCombination = false;


	float elastic = 1.f; 
	FVector windForce(0.0f, 0.0f, 0.0f);
	const int targetFPS = 30;
	const float deltaTime = 1.0 / targetFPS;
	const double targetFrameTime = 1000.0f / targetFPS; //��ǥ ������

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0f;

	//// �� �ϳ� �߰��ϰ� ����
	//FPrimitiveVector PrimitiveVector;
	//UBall* ball = new UBall();
	//PrimitiveVector.push_back(ball);
	UCamera* cam = new UCamera();

	// Main Loop 
	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);

		MSG msg;

		//�޽��� ť���� msg�� �������� ť���� ������ 
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

		//DesireNumberOfBalls = PrimitiveVector.size(); // ������Ʈ �� ���� ���� �ֽ�ȭ

		// Frame Update
		renderer.Prepare();
		renderer.PrepareShader();

		for (int i = 0; i < PrimitiveVector.size(); i++)
		{
			// ��ũ�� �󿡼��� ��ǥ�� ũ�� ���
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

		// ImGui UI ��Ʈ�� �߰��� ImGui::NewFrame()�� ImGui::Render() ���̿�
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

	// ���⿡�� ImGui �Ҹ�
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//// �Ҹ꿡 �ʿ��� �ڵ� 
	//renderer.ReleaseVertexBuffer(UBall::vertexBufferSphere);
	
	// WinMain���� ������ ���� ����
	renderer.ReleaseVertexBuffer(vertexBufferSphere); 
	renderer.ReleaseConstantBuffer();

	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();


	return 0;
}