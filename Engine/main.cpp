#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <vector>
#include <chrono>


#include <vector>

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
#include <iostream> 
#include <ctime>    
#include <cstdlib>   

HWND hWnd = nullptr;
 
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")  

//Manager
#include "InputManager.h"


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

const int winWidth = 1024;
const int winHeight = 1024;

static bool MouseClicked = false; 

// ���� �޼����� ó���� �Լ� 
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	InputManager::Input().ProcessMessage(hWnd, message, wParam, lParam);


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

// ?�성???��??�기 ?�한 ?�거??enum) ?�의
enum EAttribute
{
	WATER, // �?
	FIRE,  // �?
	GRASS  // ?�
};

// ?�성 ?�성??체크?�는 ?�우�??�수
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
	// D3D11 Device, Device Context, Swap Chain??관리하�??�한 ?�인?�들 
	ID3D11Device* Device = nullptr;						//Gpu?� ?�신 ?�기 ?�한 Device
	ID3D11DeviceContext* DeviceContext = nullptr;		// GPU 명령 ?�행???�당?�는 Context
	IDXGISwapChain* SwapChain = nullptr;				// ?�레??버터�?교체?�는 ???�용?�는 Swap Chain

	// ?�더링에 ?�요??리소??�??�태�?관리하�??�한 변?�들 
	ID3D11Texture2D* FrameBuffer = nullptr;					// ?�면 출력??
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;		// ?�스처�? ?�터 ?�겟으�??�용?�는 �?
	ID3D11RasterizerState* RasterizerState = nullptr;		// Rasterizer ?�태 (컬링, 채우�?모드 ?? 
	ID3D11Buffer* ConstantBuffer = nullptr;					// Constant Buffer (?�이?�에 ?�달???�이???�?�용)


	//UI
	ID3D11VertexShader* UIVS = nullptr;
	ID3D11PixelShader* UIPS = nullptr;
	ID3D11InputLayout* UIInputLayout = nullptr;
 	ID3D11Buffer* UIVertexBuffer = nullptr;  // ���� VB
	ID3D11Buffer* UIPerFrameCB = nullptr;
	ID3D11ShaderResourceView* UITitleSRV = nullptr;
	ID3D11ShaderResourceView* UIStartSRV = nullptr;
	ID3D11ShaderResourceView* UIExitSRV = nullptr; 

	ID3D11SamplerState* UISampler = nullptr;
	ID3D11BlendState* UIAlphaBlend = nullptr;

	FLOAT ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	D3D11_VIEWPORT ViewportInfo;	// ������ ������ �����ϴ� ����Ʈ ���� 
	 

public:

	// ?�더??초기???�수
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);

		CreateFrameBuffer();

		CreateRasterizerState();

		// depth/stencil buffer & blend state???�루지 ?�음 
	}


	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		// 지?�하??Direct3D 기능 ?�벨???�의 
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		// Swap Chain 
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width = 0; //�??�기??맞기 ?�동조정
		swapChainDesc.BufferDesc.Height = 0; //�??�기??맞기 ?�동조정	
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ?�반?�인 RGBA ?�맷
		swapChainDesc.SampleDesc.Count = 1; // 멀???�플링을 ?�용?��? ?�음
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ?�더 ?�겟으�??�용
		swapChainDesc.BufferCount = 2; // ?�블 버퍼�?
		swapChainDesc.OutputWindow = hWindow;
		swapChainDesc.Windowed = TRUE; // �?모드 
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ?�왑 방식

		// Create Dircet Deivce & Swap Chain
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
									  D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
									  featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &Device, nullptr, &DeviceContext);

		// SwapChain  ?�보 가?�오�?
		SwapChain->GetDesc(&swapChainDesc);

		// Set Viewport
		ViewportInfo = { 0.0f, 0.0f, (float)swapChainDesc.BufferDesc.Width, (float)swapChainDesc.BufferDesc.Height, 0.0f, 1.0f };
	}

	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext)
		{
			DeviceContext->Flush(); // ?�아?�는 GPU 명령??모두 ?�행
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
		// �?버처 ?�스�?가?�오�?
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		// RTV ?�성
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // ?�반?�인 RGBA ?�맷
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D ?�스처로 ?�정	

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
		rasterizerDesc.FillMode = D3D11_FILL_SOLID; // 채우�?모드  
		rasterizerDesc.CullMode = D3D11_CULL_BACK; // �??�이??컬링  

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

		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // ?�더 ?�겟을 초기??


		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0); // 1: VSync ?�성??=> GPU ?�레???�도 & ?�면 갱신 ?�도 ?�기??
	} 
	void CreateUIResources()
	{
		//���̴� ������
		ID3DBlob* vsBlob = nullptr, * psBlob = nullptr;
		D3DCompileFromFile(L"UI.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vsBlob, nullptr);
		D3DCompileFromFile(L"UI.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &psBlob, nullptr);

		Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &UIVS);
		Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &UIPS);
		
		D3D11_INPUT_ELEMENT_DESC uiInputLayout[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,       0, 8,  D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",   0,DXGI_FORMAT_R32G32B32A32_FLOAT, 0,16,  D3D11_INPUT_PER_VERTEX_DATA,0},
		};
		Device->CreateInputLayout(uiInputLayout, ARRAYSIZE(uiInputLayout),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &UIInputLayout);

		// ���� VB 
		D3D11_BUFFER_DESC vertexBufferDesc{};
		vertexBufferDesc.ByteWidth = sizeof(UIVertex) * UIVBMaxVerts;
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Device->CreateBuffer(&vertexBufferDesc, nullptr, &UIVertexBuffer);

		//Perframe CB
		D3D11_BUFFER_DESC constBufferDesc{};
		constBufferDesc.ByteWidth = 16; // float4 ���� (WindowSize(2)+pad(2))
		constBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Device->CreateBuffer(&constBufferDesc, nullptr, &UIPerFrameCB);

		// Sampler
		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		Device->CreateSamplerState(&samplerDesc, &UISampler);


		//// 6) ���� (�Ϲ� ����)
		D3D11_BLEND_DESC blendDesc{}; blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		Device->CreateBlendState(&blendDesc, &UIAlphaBlend);
		 
		LoadTextureWIC(L"ui_title.png", &UITitleSRV);
		LoadTextureWIC(L"ui_start.png", &UIStartSRV);
		LoadTextureWIC(L"ui_exit.png", &UIExitSRV);
		 
	}

	void ReleaseUIResource()
	{ 
		if (UIAlphaBlend) { UIAlphaBlend->Release(); UIAlphaBlend = nullptr; }
		if (UISampler) { UISampler->Release();    UISampler = nullptr; }
		if (UITitleSRV) { UITitleSRV->Release();   UITitleSRV = nullptr; }
		if (UIStartSRV) { UIStartSRV->Release();   UIStartSRV = nullptr; }
		if (UIExitSRV) { UIExitSRV->Release();   UIExitSRV = nullptr; }
		if (UIPerFrameCB) { UIPerFrameCB->Release(); UIPerFrameCB = nullptr; }
		if (UIVertexBuffer) { UIVertexBuffer->Release(); UIVertexBuffer = nullptr; }
		if (UIInputLayout) { UIInputLayout->Release(); UIInputLayout = nullptr; }
		if (UIPS) { UIPS->Release();         UIPS = nullptr; }
		if (UIVS) { UIVS->Release();         UIVS = nullptr; }
	}

	HRESULT LoadTextureWIC(const wchar_t* path, ID3D11ShaderResourceView** outSRV)
	{
		*outSRV = nullptr;
		IWICImagingFactory* factory = nullptr; IWICBitmapDecoder* decoder = nullptr;
		IWICBitmapFrameDecode* frame = nullptr; IWICFormatConverter* converter = nullptr;
		
		HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&factory));

		if (FAILED(hr)) return hr;
		hr = factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
		if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
		if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(&converter);
		if (SUCCEEDED(hr)) hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		
		UINT w = 0, h = 0; if (SUCCEEDED(hr)) { frame->GetSize(&w, &h); }

		std::vector<uint8_t> pixels; pixels.resize(w * h * 4);
		if (SUCCEEDED(hr)) hr = converter->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data());
		
		if (SUCCEEDED(hr)) {
			D3D11_TEXTURE2D_DESC td{}; td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
			td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
			td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			D3D11_SUBRESOURCE_DATA srd{ pixels.data(), (UINT)(w * 4), 0 };
			ID3D11Texture2D* tex = nullptr; hr = Device->CreateTexture2D(&td, &srd, &tex);
			if (SUCCEEDED(hr)) {
				D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
				sd.Format = td.Format; sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				sd.Texture2D.MipLevels = 1;
				hr = Device->CreateShaderResourceView(tex, &sd, outSRV);
				tex->Release();
			}
		}
		if (converter) converter->Release();
		if (frame) frame->Release();
		if (decoder) decoder->Release();
		if (factory) factory->Release();
		return hr;
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

		// vertesxShader???�력 ?�그?�처?� ?�환?�는지 ?�인?�야?�니�?
		// layout?�서 vertexShaderCSO�??�요로함 
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

	void PrepareShaderUI(ID3D11ShaderResourceView* UISRV)
	{ 
		DeviceContext->IASetInputLayout(UIInputLayout);
		DeviceContext->VSSetShader(UIVS, nullptr, 0);
		DeviceContext->PSSetShader(UIPS, nullptr, 0);

		DeviceContext->VSSetConstantBuffers(0, 1, &UIPerFrameCB);
		DeviceContext->PSSetConstantBuffers(0, 1, &UIPerFrameCB);

		DeviceContext->PSSetShaderResources(0, 1, &UISRV);


		DeviceContext->PSSetSamplers(0, 1, &UISampler);

		UINT stride = sizeof(UIVertex), offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &UIVertexBuffer, &stride, &offset);
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		float blendFactor[4] = { 0,0,0,0 };
		DeviceContext->OMSetBlendState(UIAlphaBlend, blendFactor, 0xffffffff);

		DeviceContext->Draw(6, 0);

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
		// 16byte ?�위�??�축?�야??
		D3D11_BUFFER_DESC constantBufferDesc = {};
		constantBufferDesc.ByteWidth = (sizeof(FConstant) + 0xf) & 0xfffffff0; // 16배수 ?�렬 
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

	void UpdateConstant(float winSize[2], float targetSize[2], bool isHovering, float ratio[2])
	{  
		if (UIPerFrameCB) 
		{
			UIInfo mainUI = { {winSize[0], winSize[1]}, isHovering ? 1 : 0, 0.0f};

			D3D11_MAPPED_SUBRESOURCE m;
			DeviceContext->Map(UIPerFrameCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
			memcpy(m.pData, &mainUI, sizeof(mainUI));
			DeviceContext->Unmap(UIPerFrameCB, 0);

			float w = targetSize[0] * (winSize[0] / winWidth), h = targetSize[1] * (winSize[1] / winHeight);
			float cx = winSize[0] * ratio[0], cy = winSize[1] * ratio[1];
			float x0 = cx - w * 0.5f, y0 = cy - h * 0.5f;
			float x1 = cx + w * 0.5f, y1 = cy + h * 0.5f;

			UIVertex verts[6] = {
			{x0,y0, 0,0, 1,1,1,1},
			{x1,y0, 1,0, 1,1,1,1},
			{x1,y1, 1,1, 1,1,1,1},
			{x0,y0, 0,0, 1,1,1,1},
			{x1,y1, 1,1, 1,1,1,1},
			{x0,y1, 0,1, 1,1,1,1},
			};

			DeviceContext->Map(UIVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
			memcpy(m.pData, verts, sizeof(verts));
			DeviceContext->Unmap(UIVertexBuffer, 0);

		} 
	}
};
 
enum class Screen { MainMenu, Running, EndingMenu, Count};
static Screen ScreenState = Screen::MainMenu;

struct MenuActions { bool start = false; bool exit = false;  };

MenuActions DrawMainMenu(URenderer& renderer, HWND hWnd)
{
	MenuActions MenuAction{};


	RECT rect;
	GetClientRect(hWnd, &rect);
	float winW = (float)(rect.right - rect.left);
	float winH = (float)(rect.bottom - rect.top);
	float winSize[2] = { winW, winH }; 

	POINT pt;
	GetCursorPos(&pt);              // ��ũ�� ��ǥ��
	ScreenToClient(hWnd, &pt);      // ������ Ŭ���̾�Ʈ ��ǥ�� ��ȯ
	int mouseX = pt.x;
	int mouseY = pt.y;
	float mousePos[2] = { mouseX, mouseY };
	 

	// Title UI   
	float titleRatio[2] = { 0.5f, 0.3f };
	float targetSize[2] = { 500, 500 };
	renderer.UpdateConstant(winSize, targetSize, true, titleRatio);
	renderer.PrepareShaderUI(renderer.UITitleSRV);

	// Start UI 
	float startRatio[2] = { 0.5f, 0.7f };
	float startUIOffset[2] = { 50.0f, 100.f };
	targetSize[0] = 200; targetSize[1] = 200;
	float hoveringSize[2] = { targetSize[0] - startUIOffset[0], targetSize[1] - startUIOffset[1] };

	UIReact reactStart = MakeRect(winSize, hoveringSize, startRatio);
	bool startHoverTest = CheckMouseOnUI(reactStart, mouseX, mouseY);
	renderer.UpdateConstant(winSize, targetSize, startHoverTest, startRatio);
	renderer.PrepareShaderUI(renderer.UIStartSRV);


	// Exit UI 
	float exitRatio[2] = { 0.5f, 0.8f };
	float exitUIOffset[2] = { 50.0f, 100.0f };
	targetSize[0] = 200; targetSize[1] = 200;
	hoveringSize[0] = targetSize[0] - exitUIOffset[0]; hoveringSize[1] = targetSize[1] - exitUIOffset[1];

	UIReact reactExit = MakeRect(winSize, hoveringSize, exitRatio);
	bool exitHoverTest = CheckMouseOnUI(reactExit, mouseX, mouseY);
	renderer.UpdateConstant(winSize, targetSize, exitHoverTest, exitRatio);
	renderer.PrepareShaderUI(renderer.UIExitSRV);

	// ====== Ŭ�� ó�� ====== 
	if (InputManager::Input().IsClicked(MouseButton::Left) && startHoverTest)
	{
		MenuAction.start = true;
	}
	if (InputManager::Input().IsClicked(MouseButton::Left) && exitHoverTest)
	{
		MenuAction.exit = true;
		//NewController->bIsEnabled = true;
	}

	return MenuAction; 
}
  
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

class UCamera
{
public:
	FVector Location;
	float RenderScale = 1.0f;
	float TargetRenderScale = 1.0f;

	float RefRadius = 0.2f;    // RenderScale = 1.0 기�?
	float MinScale = 0.15f;    // ?��????�한??(?�무 ?�아?�서 ?�처??보이지 ?�게)
	float MaxScale = 2.0f;     // ?��????�한??(과도??�???방�?)
	float SmoothT = 0.2f;      // Lerp 비율
public:
	void SetLocation(FVector location)
	{
		this->Location = location;
	}

	void UpdateCamera(UPrimitive* Player)
	{
		SetLocation(Player->GetLocation());

		// ?�제 ?�더�??��??�을 목표 ?��??�을 ?�해 ?�진?�으�?조정
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
};

class UEnemy : public UPrimitive
{
public:
	// ?�성?? ENEMY ?�성 ???�출??
	UEnemy()
	{
		// 무작???�성, ?�치, ?�도, ?�기 ?�정
		Attribute = (EAttribute)(rand() % 3);
		Location = GetRandomLocationOusideScreen();

		const float enemySpeed = 0.001f;
		Velocity.x = (float)(rand() % 100 - 50) * enemySpeed;
		Velocity.y = (float)(rand() % 100 - 50) * enemySpeed;

		Radius = ((rand() / (float)RAND_MAX)) * 0.1f + 0.03f;  // 최소, 최�? ?�기 지??
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

		if (Location.x > 1.0f - Radius || Location.x < -1.0f + Radius)
		{
			Velocity.x *= -1.0f;
		}
		if (Location.y > 1.0f - Radius || Location.y < -1.0f + Radius)
		{
			Velocity.y *= -1.0f;
		}
	}

	FVector GetRandomLocationOusideScreen()
	{
		FVector Vector;
		do
		{
			Vector.x = (static_cast<float>(rand()) / RAND_MAX) * 4.0f - 2.0f; // -2 ~ 2
			Vector.y = (static_cast<float>(rand()) / RAND_MAX) * 4.0f - 2.0f; // -2 ~ 2
			Vector.z = 0.0f; // Z??고정
		}
		// [-1, 1] ?�사각형 ?��??��? 직접 조건 체크
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
		Location = FVector((rand() / (float)RAND_MAX) * 2.0f - 1.0f, (rand() / (float)RAND_MAX) * 2.0f - 1.0f, 0.0f);
		Velocity = FVector(0.0f, 0.0f, 0.0f); // ?�직이지 ?�으므�??�도??0
		Radius = ((rand() / (float)RAND_MAX)) * 0.05f + 0.02f;
		Mass = Radius * 10.0f;
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
};

//int UBall::TotalBalls = 0;
//ID3D11Buffer* UBall::vertexBufferSphere = nullptr; // static 멤버 변??초기??


class Controller
{
public:
	UPlayer** PlayerCells = nullptr;
	int MaxCount = 0;
	int Count = 0;

public:
	Controller(int MaxCount)
	{
		MaxCount = 4;
		Count = 0;
		PlayerCells = new UPlayer * [MaxCount];
		for (int i = 0; i < MaxCount; ++i)
		{
			PlayerCells[i] = nullptr;
		}
	}

	~Controller()
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
			// NRE ?��?
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

		// Fallback: 질량 ?�이 거의 0??경우 ?�차 방�? 차원?�서 ?�균 ?�치 ?�용
		if (TotalMass < 1e-6)
		{
			CenterX = CenterY = 0.0f;
			for (int i = 0; i < Count; ++i)
			{
				// NRE ?��?
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
		if (Player == nullptr) return; // ?�레?�어가 ?�으�??�무것도 ????

		// 배열??거꾸�??�회?�야 객체 ?�거 ?�에???�전??
		for (int i = Size - 1; i >= 0; --i)
		{
			// ?�기 ?�신(?�레?�어)과는 충돌 검?��? ?��? ?�음
			if (primitives[i] == Player)
			{
				continue;
			}

			UPrimitive* other = primitives[i];
			float dist2 = FVector::Distance2(Player->GetLocation(), other->GetLocation());
			float minDist = Player->GetRadius() + other->GetRadius();

			// 충돌?�다�?
			if (dist2 < minDist * minDist)
			{
				// ?�기???�성?��? 체크
				if (CheckWin(Player->GetAttribute(), other->GetAttribute()))
				{
					Player->AddScore(10);
					Player->SetRadius(Player->GetRadius() + 0.005f); // ũ�� ����
				}
				else // 지거나 비기???�성
				{
					Player->AddScore(-5);
					Player->SetRadius(Player->GetRadius() - 0.005f); // ũ�� ����
				}

				// 충돌??객체???�거
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

	//			// 구�? 구의 충돌처리 sqrt 비용??비싸�??�문??squre?�어?�는 ?�태?�서 거리 비교
	//			if (dist2 < minDist * minDist)
	//			{

	//				if (bCombination && mergeCount < 1024)
	//				{
	//					mergeList[mergeCount] = { i, j };
	//					mergeCount++;
	//				}
	//				//Combine???�니거나 mergeCount가 최�? merge보다 ????
	//				else
	//				{
	//					float dist = sqrt(dist2);
	//					FVector normal = (pos2 - pos1);
	//					normal.Normalize();

	//					FVector velocityOfA = a->GetVelocity();
	//					FVector velocityOfB = b->GetVelocity();

	//					FVector relativeVelocity = velocityOfB - velocityOfA;
	//					float speed = Dot(relativeVelocity, normal);

	//					// ?��? ?�도?� 충돌??구�???방향??같을 ?�만 충격??계산
	//					if (speed < 0.0f)
	//					{
	//						float massOfA = a->GetMass();
	//						float massOfB = b->GetMass();

	//						float impulse = -(1 + elastic) * speed / (1 / massOfA + 1 / massOfB);
	//						FVector J = normal * impulse;

	//						a->SetVelocity(velocityOfA - J / massOfA);
	//						b->SetVelocity(velocityOfB + J / massOfB);
	//					}

	//					// ?�치 보정
	//					float penetration = minDist - dist;
	//					FVector correction = normal * (penetration * 0.5f);
	//					a->SetLocation(pos1 - correction);
	//					b->SetLocation(pos2 + correction);
	//				}
	//			}
	//		}
	//	}
	//}

	// 쿨룽 ?�으�??�기???�과 
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

	////?�정 반�?�??�하????분할?�도�??�도
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

	//			//?�로??�??�성
	//			UPrimitive* newBall1 = new UBall(radius);
	//			newBall1->SetLocation(FVector(location.x + radius, location.y, location.z));
	//			newBall1->SetVelocity(FVector(velocity.x + 0.01f, velocity.y, velocity.z));
	//			push_back(newBall1);

	//			UPrimitive* newBall2 = new UBall(radius);
	//			newBall2->SetLocation(FVector(location.x - radius, location.y, location.z));
	//			newBall2->SetVelocity(FVector(velocity.x - 0.01f, velocity.y, velocity.z));
	//			push_back(newBall2);

	//			//기존 �??�거
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
	//	//??index먼�? 처리�??�해???�렬
	//	for (int i = 0; i < mergeCount - 1; ++i)
	//	{
	//		for (int j = i + 1; j < mergeCount; ++j)
	//		{
	//			int maxI = mergeList[i].indexA > mergeList[i].indexB ? mergeList[i].indexA : mergeList[i].indexB;
	//			int maxJ = mergeList[j].indexA > mergeList[j].indexB ? mergeList[j].indexA : mergeList[j].indexB;

	//			if (maxI < maxJ) // ?�림차순 ?�렬
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

	//		//?�에 ?�는 index 중에?�도 ??값먼?� 계산?�기 ?�함
	//		if (indexA < indexB)
	//		{
	//			int temp = indexA;
	//			indexA = indexB;
	//			indexB = temp;
	//		}

	//		//?�외처리 
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
			FVector renderedLocation = camera->GetCameraSpaceLocation(primitives[i]);
			float renderedRadius = camera->GetCameraSpaceRadius(primitives[i]);
			
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

using Clock = std::chrono::steady_clock;
Clock::time_point nextSpawn;

void InitSpawnerChrono()
{
	std::srand(static_cast<unsigned>(GetTickCount64()));
	auto ms = 300 + (std::rand() % 300);
	nextSpawn = Clock::now() + std::chrono::milliseconds(ms);
}

void TickSpawnerChrono(FPrimitiveVector* PrimitiveVector)
{
	auto now = Clock::now();
	if (now >= nextSpawn)
	{
		UEnemy* Enemy = new UEnemy();
		PrimitiveVector->push_back(Enemy);

		auto ms = 300 + (std::rand() % 300);
		nextSpawn = now + std::chrono::milliseconds(ms);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{ 
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// ������ Ŭ���� �̸� 
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ?�도???�?��?�??�름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메세지�?처리???�수??WndProc???�수 ?�인?��? WindowClass 구조체에 ?�록
	WNDCLASSW wndClass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// ?�도???�래???�록
	RegisterClassW(&wndClass);

	// 1024 * 1024 ?�기???�도???�성
	hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024, nullptr, nullptr, hInstance, nullptr);

	srand((unsigned int)time(NULL));

	//각종 ?�성/초기??코드�??�기??추�?
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

	// ?�레?�어 ?�성 �?추�?
	UPlayer* player = new UPlayer();
	PrimitiveVector.push_back(player);

	// 초기 ENEMY?� PREY ?�성
	for (int i = 0; i < 10; ++i)
	{
		PrimitiveVector.push_back(new UEnemy());
		PrimitiveVector.push_back(new UPrey());
	}

	//// 구에 관??Vertex Buffer????번만 ?�성 ???�사??
	//UBall::vertexBufferSphere = renderer.CreateVertexBuffer(verticesSphere, sizeof(sphere_vertices));

	bool bIsExit = false;
	bool bTestEnabled = false;

	static bool prevLButton = false; 

	bool bMagnetic = false;

	bool bExplosion = false;
	bool bCombination = false;


	float elastic = 1.f;
	FVector windForce(0.0f, 0.0f, 0.0f);
	const int targetFPS = 30;
	const float deltaTime = 1.0 / targetFPS;
	const double targetFrameTime = 1000.0f / targetFPS; //목표 ?�레??

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0f;

	LARGE_INTEGER CreateStartTime, CurrentTime;
	double CreateInterval = rand() % 1000 + 500;
	QueryPerformanceCounter(&CreateStartTime);
	//// �??�나 추�??�고 ?�작
	//FPrimitiveVector PrimitiveVector;
	//UBall* ball = new UBall();
	//PrimitiveVector.push_back(ball);
	UCamera* cam = new UCamera();

	// Dirty ?�래그�? ?�한 벡터 - ?�더링할 객체???�덱?�만 ?�??
	std::vector<int> InsidePrimitives;

	InitSpawnerChrono();

	//Create UI Texture 
	renderer.CreateUIResources();

	// Main Loop 
	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);

		MSG msg;

		//Init InputManger 
		InputManager::Input().BeginFrame();

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
		PrimitiveVector.ProcessGameLogic();

		// 2. 카메?��? ?�레?�어�??�라가?�록 ?�데?�트
		cam->UpdateCamera(PrimitiveVector.Player);


		// Frame Update
		renderer.Prepare();
		renderer.PrepareShader();

		cam->UpdateCamera(PrimitiveVector[0]);

		// Dirty ?�래�??�데?�트 �?�?보더 밖의 객체 ??��
		std::vector<int> OutsidePrimitives; // ??��??객체?�의 ?�덱??

		// FPrimitiveVector???�수�??�용?�여 보더 분류
		PrimitiveVector.ClassifyBorder(cam, InsidePrimitives, OutsidePrimitives);

		// �?보더 밖의 객체????��
		PrimitiveVector.RemoveOutsidePrimitives(OutsidePrimitives);

		// 보더 ?�의 객체?�만 ?�더�?
		for (int idx : InsidePrimitives)
		{
			// ?�크�??�에?�의 좌표?� ?�기 계산
			UPrimitive* prim = PrimitiveVector[idx];
			if (prim != nullptr)
			{
				FVector renderedLocation = cam->GetCameraSpaceLocation(prim);
				float renderedRadius = cam->GetCameraSpaceRadius(prim);
				renderer.UpdateConstant(renderedLocation, renderedRadius);
				renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
			}
		}
		TickSpawnerChrono(&PrimitiveVector);

		////////// UI TEST ////////// 

		switch (ScreenState)
		{
		case Screen::MainMenu:
		{
			MenuActions action = DrawMainMenu(renderer, hWnd);
			if (action.start)
				ScreenState = Screen::Running;

			if (action.exit)
				bIsExit = true;

			break;
		}
		case Screen::Running:

			break;

		case Screen::EndingMenu:
			break;
		}


		////////// UI TEST ////////// 

		// ImGui Update
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Game Info");
		ImGui::Text("Score: %d", PrimitiveVector.Player ? PrimitiveVector.Player->GetScore() : 0);
		ImGui::Text("Objects: %d", PrimitiveVector.size());

		// ?�레?�어 ?�성???�스?�로 보여주기
		if (PrimitiveVector.Player)
		{
			EAttribute attr = PrimitiveVector.Player->GetAttribute();
			const char* attrText = (attr == WATER) ? "WATER" : (attr == FIRE) ? "FIRE" : "GRASS";
			ImGui::Text("Player Attribute: %s", attrText);
		}
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
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		//// ?�멸???�요??코드 
		//renderer.ReleaseVertexBuffer(UBall::vertexBufferSphere);
		
		// WinMain?�서 ?�성??버퍼 ?�제
		renderer.ReleaseVertexBuffer(vertexBufferSphere);
		renderer.ReleaseConstantBuffer();

		renderer.ReleaseShader();
		renderer.Release();

	return 0;
}