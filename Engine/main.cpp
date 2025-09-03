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

// °¢Á¾ ¸Ş¼¼Áö¸¦ Ã³¸®ÇÒ ÇÔ¼ö 
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

// ?ì„±???˜í??´ê¸° ?„í•œ ?´ê±°??enum) ?•ì˜
enum EAttribute
{
	WATER, // ë¬?
	FIRE,  // ë¶?
	GRASS  // ?€
};

// ?ì„± ?ì„±??ì²´í¬?˜ëŠ” ?„ìš°ë¯??¨ìˆ˜
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
	// D3D11 Device, Device Context, Swap Chain??ê´€ë¦¬í•˜ê¸??„í•œ ?¬ì¸?°ë“¤ 
	ID3D11Device* Device = nullptr;						//Gpu?€ ?µì‹  ?˜ê¸° ?„í•œ Device
	ID3D11DeviceContext* DeviceContext = nullptr;		// GPU ëª…ë ¹ ?¤í–‰???´ë‹¹?˜ëŠ” Context
	IDXGISwapChain* SwapChain = nullptr;				// ?„ë ˆ??ë²„í„°ë¥?êµì²´?˜ëŠ” ???¬ìš©?˜ëŠ” Swap Chain

	// ?Œë”ë§ì— ?„ìš”??ë¦¬ì†Œ??ë°??íƒœë¥?ê´€ë¦¬í•˜ê¸??„í•œ ë³€?˜ë“¤ 
	ID3D11Texture2D* FrameBuffer = nullptr;					// ?”ë©´ ì¶œë ¥??
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;		// ?ìŠ¤ì²˜ë? ?Œí„° ?€ê²Ÿìœ¼ë¡??¬ìš©?˜ëŠ” ë·?
	ID3D11RasterizerState* RasterizerState = nullptr;		// Rasterizer ?íƒœ (ì»¬ë§, ì±„ìš°ê¸?ëª¨ë“œ ?? 
	ID3D11Buffer* ConstantBuffer = nullptr;					// Constant Buffer (?°ì´?”ì— ?„ë‹¬???°ì´???€?¥ìš©)


	//UI
	ID3D11VertexShader* UIVS = nullptr;
	ID3D11PixelShader* UIPS = nullptr;
	ID3D11InputLayout* UIInputLayout = nullptr;
 	ID3D11Buffer* UIVertexBuffer = nullptr;  // µ¿Àû VB
	ID3D11Buffer* UIPerFrameCB = nullptr;
	ID3D11ShaderResourceView* UITitleSRV = nullptr;
	ID3D11ShaderResourceView* UIStartSRV = nullptr;
	ID3D11ShaderResourceView* UIExitSRV = nullptr; 

	ID3D11SamplerState* UISampler = nullptr;
	ID3D11BlendState* UIAlphaBlend = nullptr;

	FLOAT ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	D3D11_VIEWPORT ViewportInfo;	// ·»´õ¸µ ¿µ¿ªÀ» Á¤ÀÇÇÏ´Â ºäÆ÷Æ® Á¤º¸ 
	 

public:

	// ?Œë”??ì´ˆê¸°???¨ìˆ˜
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);

		CreateFrameBuffer();

		CreateRasterizerState();

		// depth/stencil buffer & blend state???¤ë£¨ì§€ ?ŠìŒ 
	}


	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		// ì§€?í•˜??Direct3D ê¸°ëŠ¥ ?ˆë²¨???•ì˜ 
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		// Swap Chain 
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width = 0; //ì°??¬ê¸°??ë§ê¸° ?ë™ì¡°ì •
		swapChainDesc.BufferDesc.Height = 0; //ì°??¬ê¸°??ë§ê¸° ?ë™ì¡°ì •	
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ?¼ë°˜?ì¸ RGBA ?¬ë§·
		swapChainDesc.SampleDesc.Count = 1; // ë©€???˜í”Œë§ì„ ?¬ìš©?˜ì? ?ŠìŒ
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ?Œë” ?€ê²Ÿìœ¼ë¡??¬ìš©
		swapChainDesc.BufferCount = 2; // ?”ë¸” ë²„í¼ë§?
		swapChainDesc.OutputWindow = hWindow;
		swapChainDesc.Windowed = TRUE; // ì°?ëª¨ë“œ 
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ?¤ì™‘ ë°©ì‹

		// Create Dircet Deivce & Swap Chain
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
									  D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
									  featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &Device, nullptr, &DeviceContext);

		// SwapChain  ?•ë³´ ê°€?¸ì˜¤ê¸?
		SwapChain->GetDesc(&swapChainDesc);

		// Set Viewport
		ViewportInfo = { 0.0f, 0.0f, (float)swapChainDesc.BufferDesc.Width, (float)swapChainDesc.BufferDesc.Height, 0.0f, 1.0f };
	}

	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext)
		{
			DeviceContext->Flush(); // ?¨ì•„?ˆëŠ” GPU ëª…ë ¹??ëª¨ë‘ ?¤í–‰
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
		// ë°?ë²„ì²˜ ?ìŠ¤ì²?ê°€?¸ì˜¤ê¸?
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		// RTV ?ì„±
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // ?¼ë°˜?ì¸ RGBA ?¬ë§·
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D ?ìŠ¤ì²˜ë¡œ ?¤ì •	

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
		rasterizerDesc.FillMode = D3D11_FILL_SOLID; // ì±„ìš°ê¸?ëª¨ë“œ  
		rasterizerDesc.CullMode = D3D11_CULL_BACK; // ë°??˜ì´??ì»¬ë§  

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

		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // ?Œë” ?€ê²Ÿì„ ì´ˆê¸°??


		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0); // 1: VSync ?œì„±??=> GPU ?„ë ˆ???ë„ & ?”ë©´ ê°±ì‹  ?ë„ ?™ê¸°??
	} 
	void CreateUIResources()
	{
		//½¦ÀÌ´õ ÄÄÆÄÀÏ
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

		// µ¿Àû VB 
		D3D11_BUFFER_DESC vertexBufferDesc{};
		vertexBufferDesc.ByteWidth = sizeof(UIVertex) * UIVBMaxVerts;
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Device->CreateBuffer(&vertexBufferDesc, nullptr, &UIVertexBuffer);

		//Perframe CB
		D3D11_BUFFER_DESC constBufferDesc{};
		constBufferDesc.ByteWidth = 16; // float4 Á¤·Ä (WindowSize(2)+pad(2))
		constBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Device->CreateBuffer(&constBufferDesc, nullptr, &UIPerFrameCB);

		// Sampler
		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		Device->CreateSamplerState(&samplerDesc, &UISampler);


		//// 6) ºí·»µå (ÀÏ¹İ ¾ËÆÄ)
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

		// vertesxShader???…ë ¥ ?œê·¸?ˆì²˜?€ ?¸í™˜?˜ëŠ”ì§€ ?•ì¸?´ì•¼?˜ë‹ˆê¹?
		// layout?ì„œ vertexShaderCSOë¥??„ìš”ë¡œí•¨ 
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
		// 16byte ?¨ìœ„ë¡??•ì¶•?´ì•¼??
		D3D11_BUFFER_DESC constantBufferDesc = {};
		constantBufferDesc.ByteWidth = (sizeof(FConstant) + 0xf) & 0xfffffff0; // 16ë°°ìˆ˜ ?•ë ¬ 
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
	GetCursorPos(&pt);              // ½ºÅ©¸° ÁÂÇ¥°è
	ScreenToClient(hWnd, &pt);      // À©µµ¿ì Å¬¶óÀÌ¾ğÆ® ÁÂÇ¥·Î º¯È¯
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

	// ====== Å¬¸¯ Ã³¸® ====== 
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

	float RefRadius = 0.2f;    // RenderScale = 1.0 ê¸°ì?
	float MinScale = 0.15f;    // ?¤ì????˜í•œ??(?ˆë¬´ ?‘ì•„?¸ì„œ ?ì²˜??ë³´ì´ì§€ ?Šê²Œ)
	float MaxScale = 2.0f;     // ?¤ì????í•œ??(ê³¼ë„??ì¤???ë°©ì?)
	float SmoothT = 0.2f;      // Lerp ë¹„ìœ¨
public:
	void SetLocation(FVector location)
	{
		this->Location = location;
	}

	void UpdateCamera(UPrimitive* Player)
	{
		SetLocation(Player->GetLocation());

		// ?¤ì œ ?Œë”ë§??¤ì??¼ì„ ëª©í‘œ ?¤ì??¼ì„ ?¥í•´ ?ì§„?ìœ¼ë¡?ì¡°ì •
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
	// ?ì„±?? ?Œë ˆ?´ì–´ ?ì„± ???¸ì¶œ??
	UPlayer()
	{
		// 1. ï¿½Ã·ï¿½ï¿½Ì¾ï¿½ ï¿½Ó¼ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
		Attribute = (EAttribute)(rand() % 3); // 0, 1, 2 ì¤??˜ë‚˜ë¥??œë¤?¼ë¡œ ë½‘ì•„ ?ì„±?¼ë¡œ ì§€??

		Radius = 0.08f; // ï¿½Ê±ï¿½ Å©ï¿½ï¿½
		Mass = Radius * 10.0f;
		Location = FVector(0.0f, 0.0f, 0.0f); // ?”ë©´ ì¤‘ì•™?ì„œ ?œì‘
		Velocity = FVector(0.0f, 0.0f, 0.0f); // ?ë„??ë§ˆìš°?¤ë? ?°ë¥´ë¯€ë¡?0?¼ë¡œ ?œì‘
		Score = 0;
	}

	// UPrimitive??ê·œì¹™(?œìˆ˜ ê°€???¨ìˆ˜)???°ë¼ ëª¨ë“  ?¨ìˆ˜ë¥?êµ¬í˜„
	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }

	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }

	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// ê·œì¹™???°ë¼ GetAttribute ?¨ìˆ˜ë¥?êµ¬í˜„
	virtual EAttribute GetAttribute() const override { return Attribute; }
	// ?¬ìš©?˜ì? ?ŠëŠ” ê¸°ëŠ¥?¤ì? ê¸°ë³¸ ?•íƒœë¡?êµ¬í˜„
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// ?Œë ˆ?´ì–´???µì‹¬ ë¡œì§: ë§ˆìš°?¤ë? ?°ë¼ ?€ì§ì„
	virtual void Movement() override
	{
		extern HWND hWnd; // WinMain??hWndë¥??¸ë??ì„œ ì°¸ì¡°

		POINT mousePos;
		GetCursorPos(&mousePos); // ë§ˆìš°?¤ì˜ ?¤í¬ë¦?ì¢Œí‘œë¥??»ìŒ
		ScreenToClient(hWnd, &mousePos); // ?¤í¬ë¦?ì¢Œí‘œë¥??„ë¡œê·¸ë¨ ì°??´ë? ì¢Œí‘œë¡?ë³€??

		RECT clientRect;
		GetClientRect(hWnd, &clientRect); // ?„ë¡œê·¸ë¨ ì°½ì˜ ?¬ê¸°ë¥??»ìŒ

		// ì°??´ë? ì¢Œí‘œ(e.g., 0~1024)ë¥?ê²Œì„ ?”ë“œ ì¢Œí‘œ(-1.0 ~ 1.0)ë¡?ë³€??
		float worldX = ((float)mousePos.x / clientRect.right) * 2.0f - 1.0f;
		float worldY = (-(float)mousePos.y / clientRect.bottom) * 2.0f + 1.0f;

		Location.x = worldX;
		Location.y = worldY;
	}

	// ?¬ê¸°?€ ?ìˆ˜ë¥?ì¡°ì ˆ?˜ëŠ” ?ˆë¡œ???¨ìˆ˜??
	void AddScore(int amount) { Score += amount; }
	int GetScore() const { return Score; }
	void SetRadius(float newRadius)
	{
		Radius = newRadius;
		// ?¬ê¸°ê°€ ë³€?˜ë©´ ì§ˆëŸ‰??ê°™ì´ ?…ë°?´íŠ¸
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
	// ?ì„±?? ENEMY ?ì„± ???¸ì¶œ??
	UEnemy()
	{
		// ë¬´ì‘???ì„±, ?„ì¹˜, ?ë„, ?¬ê¸° ?¤ì •
		Attribute = (EAttribute)(rand() % 3);
		Location = GetRandomLocationOusideScreen();

		const float enemySpeed = 0.001f;
		Velocity.x = (float)(rand() % 100 - 50) * enemySpeed;
		Velocity.y = (float)(rand() % 100 - 50) * enemySpeed;

		Radius = ((rand() / (float)RAND_MAX)) * 0.1f + 0.03f;  // ìµœì†Œ, ìµœë? ?¬ê¸° ì§€??
		Mass = Radius * 10.0f;
	}

	// UPrimitive??ê·œì¹™???°ë¼ ëª¨ë“  ?¨ìˆ˜ë¥?êµ¬í˜„

	virtual FVector GetLocation() const override { return Location; }
	virtual void SetLocation(FVector newLocation) override { Location = newLocation; }
	virtual FVector GetVelocity() const override { return Velocity; }
	virtual void SetVelocity(FVector newVelocity) override { Velocity = newVelocity; }
	virtual float GetMass() const override { return Mass; }
	virtual float GetRadius() const override { return Radius; }
	// ê·œì¹™???°ë¼ GetAttribute ?¨ìˆ˜ë¥?êµ¬í˜„
	virtual EAttribute GetAttribute() const override { return Attribute; }
	virtual float GetMagnetic() const override { return 0.0f; }
	virtual bool GetDivide() const override { return false; }
	virtual void SetDivide(bool newDivide) override {}

	// ENEMY???€ì§ì„: ê¸°ì¡´ UBallì²˜ëŸ¼ ë²½ì— ?•ê?
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
			Vector.z = 0.0f; // Z??ê³ ì •
		}
		// [-1, 1] ?•ì‚¬ê°í˜• ?´ë??¸ì? ì§ì ‘ ì¡°ê±´ ì²´í¬
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
	// ?ì„±?? PREY ?ì„± ???¸ì¶œ??
	UPrey()
	{
		// ë¬´ì‘???ì„±, ?„ì¹˜, ?¬ê¸° ?¤ì •
		Attribute = (EAttribute)(rand() % 3);
		Location = FVector((rand() / (float)RAND_MAX) * 2.0f - 1.0f, (rand() / (float)RAND_MAX) * 2.0f - 1.0f, 0.0f);
		Velocity = FVector(0.0f, 0.0f, 0.0f); // ?€ì§ì´ì§€ ?Šìœ¼ë¯€ë¡??ë„??0
		Radius = ((rand() / (float)RAND_MAX)) * 0.05f + 0.02f;
		Mass = Radius * 10.0f;
	}

	// UPrimitive??ê·œì¹™???°ë¼ ëª¨ë“  ?¨ìˆ˜ë¥?êµ¬í˜„
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

	// PREY???€ì§ì„: ?€ì§ì´ì§€ ?ŠìŒ
	virtual void Movement() override
	{
		// ?„ë¬´ ì½”ë“œ???†ìŒ
	}

public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	EAttribute Attribute;
};

//int UBall::TotalBalls = 0;
//ID3D11Buffer* UBall::vertexBufferSphere = nullptr; // static ë©¤ë²„ ë³€??ì´ˆê¸°??


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
			// NRE ?€ë¹?
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

		// Fallback: ì§ˆëŸ‰ ?©ì´ ê±°ì˜ 0??ê²½ìš° ?¤ì°¨ ë°©ì? ì°¨ì›?ì„œ ?‰ê·  ?„ì¹˜ ?¬ìš©
		if (TotalMass < 1e-6)
		{
			CenterX = CenterY = 0.0f;
			for (int i = 0; i < Count; ++i)
			{
				// NRE ?€ë¹?
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
	// ?Œë ˆ?´ì–´ ê°ì²´???½ê²Œ ?‘ê·¼?˜ê¸° ?„í•œ ?¬ì¸??
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

		// ë§Œì•½ ì¶”ê??˜ëŠ” ê°ì²´ê°€ ?Œë ˆ?´ì–´?¼ë©´, Player ?¬ì¸?°ì— ?€??
		// dynamic_cast??UPrimitive*ë¥?UPlayer*ë¡??ˆì „?˜ê²Œ ë³€???œë„
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
		// ? íš¨?˜ì? ?Šì? ?¸ë±?¤ë©´ ì¦‰ì‹œ ì¢…ë£Œ
		if (index < 0 || index >= Size)
		{
			return;
		}

		// ê°ì²´ ë©”ëª¨ë¦??´ì œ
		delete primitives[index];

		// ë§ˆì?ë§??ì†Œë¥??„ì¬ ?„ì¹˜ë¡??´ë™ (ê°€??ë¹ ë¥¸ ?œê±° ë°©ë²•)
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

	int size() const // ìº¡ìŠ?”ë? ?€ë¹„í•´???¨ìˆ˜ë¡??‘ê·¼
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
		if (Player == nullptr) return; // ?Œë ˆ?´ì–´ê°€ ?†ìœ¼ë©??„ë¬´ê²ƒë„ ????

		// ë°°ì—´??ê±°ê¾¸ë¡??œíšŒ?´ì•¼ ê°ì²´ ?œê±° ?œì—???ˆì „??
		for (int i = Size - 1; i >= 0; --i)
		{
			// ?ê¸° ?ì‹ (?Œë ˆ?´ì–´)ê³¼ëŠ” ì¶©ëŒ ê²€?¬ë? ?˜ì? ?ŠìŒ
			if (primitives[i] == Player)
			{
				continue;
			}

			UPrimitive* other = primitives[i];
			float dist2 = FVector::Distance2(Player->GetLocation(), other->GetLocation());
			float minDist = Player->GetRadius() + other->GetRadius();

			// ì¶©ëŒ?ˆë‹¤ë©?
			if (dist2 < minDist * minDist)
			{
				// ?´ê¸°???ì„±?¸ì? ì²´í¬
				if (CheckWin(Player->GetAttribute(), other->GetAttribute()))
				{
					Player->AddScore(10);
					Player->SetRadius(Player->GetRadius() + 0.005f); // Å©ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
				}
				else // ì§€ê±°ë‚˜ ë¹„ê¸°???ì„±
				{
					Player->AddScore(-5);
					Player->SetRadius(Player->GetRadius() - 0.005f); // Å©ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
				}

				// ì¶©ëŒ??ê°ì²´???œê±°
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

	//			// êµ¬ì? êµ¬ì˜ ì¶©ëŒì²˜ë¦¬ sqrt ë¹„ìš©??ë¹„ì‹¸ê¸??Œë¬¸??squre?˜ì–´?ˆëŠ” ?íƒœ?ì„œ ê±°ë¦¬ ë¹„êµ
	//			if (dist2 < minDist * minDist)
	//			{

	//				if (bCombination && mergeCount < 1024)
	//				{
	//					mergeList[mergeCount] = { i, j };
	//					mergeCount++;
	//				}
	//				//Combine???„ë‹ˆê±°ë‚˜ mergeCountê°€ ìµœë? mergeë³´ë‹¤ ????
	//				else
	//				{
	//					float dist = sqrt(dist2);
	//					FVector normal = (pos2 - pos1);
	//					normal.Normalize();

	//					FVector velocityOfA = a->GetVelocity();
	//					FVector velocityOfB = b->GetVelocity();

	//					FVector relativeVelocity = velocityOfB - velocityOfA;
	//					float speed = Dot(relativeVelocity, normal);

	//					// ?ë? ?ë„?€ ì¶©ëŒ??êµ¬ì???ë°©í–¥??ê°™ì„ ?Œë§Œ ì¶©ê²©??ê³„ì‚°
	//					if (speed < 0.0f)
	//					{
	//						float massOfA = a->GetMass();
	//						float massOfB = b->GetMass();

	//						float impulse = -(1 + elastic) * speed / (1 / massOfA + 1 / massOfB);
	//						FVector J = normal * impulse;

	//						a->SetVelocity(velocityOfA - J / massOfA);
	//						b->SetVelocity(velocityOfB + J / massOfB);
	//					}

	//					// ?„ì¹˜ ë³´ì •
	//					float penetration = minDist - dist;
	//					FVector correction = normal * (penetration * 0.5f);
	//					a->SetLocation(pos1 - correction);
	//					b->SetLocation(pos2 + correction);
	//				}
	//			}
	//		}
	//	}
	//}

	// ì¿¨ë£½ ?ìœ¼ë¡??ê¸°???¨ê³¼ 
	// ê±°ë¦¬ ?œê³±??ë°˜ë¹„ë¡€
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

				float k = 0.001f; //?˜ì¹˜ ?ˆì •???„í•´???„ì˜ë¡?ì¡°ì •
				float q1 = a->GetMagnetic();
				float q2 = b->GetMagnetic();
				float dist2 = FVector::Distance2(a->GetLocation(), b->GetLocation());

				dist2 = dist2 == 0 ? 1e-5 : dist2;
				float force = q1 * q2 * k / dist2;

				FVector normal = a->GetLocation() - b->GetLocation();
				//normal.Normalize(); // normalize?´ì£¼ë©??ˆë¬´ ì»¤ì§ˆ ???ˆìœ¼???¨ìŠ¤

				totalMagneticForce += normal * force;
			}
			FVector acceleration = totalMagneticForce / a->GetMass();

			a->SetVelocity(a->GetVelocity() + acceleration);
		}
	}

	////?¼ì • ë°˜ì?ë¦??´í•˜????ë¶„í• ?˜ë„ë¡?? ë„
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

	//			//?ˆë¡œ??ê³??ì„±
	//			UPrimitive* newBall1 = new UBall(radius);
	//			newBall1->SetLocation(FVector(location.x + radius, location.y, location.z));
	//			newBall1->SetVelocity(FVector(velocity.x + 0.01f, velocity.y, velocity.z));
	//			push_back(newBall1);

	//			UPrimitive* newBall2 = new UBall(radius);
	//			newBall2->SetLocation(FVector(location.x - radius, location.y, location.z));
	//			newBall2->SetVelocity(FVector(velocity.x - 0.01f, velocity.y, velocity.z));
	//			push_back(newBall2);

	//			//ê¸°ì¡´ ê³??œê±°
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
	//	//??indexë¨¼ì? ì²˜ë¦¬ë¥??„í•´???•ë ¬
	//	for (int i = 0; i < mergeCount - 1; ++i)
	//	{
	//		for (int j = i + 1; j < mergeCount; ++j)
	//		{
	//			int maxI = mergeList[i].indexA > mergeList[i].indexB ? mergeList[i].indexA : mergeList[i].indexB;
	//			int maxJ = mergeList[j].indexA > mergeList[j].indexB ? mergeList[j].indexA : mergeList[j].indexB;

	//			if (maxI < maxJ) // ?´ë¦¼ì°¨ìˆœ ?•ë ¬
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

	//		//?ˆì— ?ˆëŠ” index ì¤‘ì—?œë„ ??ê°’ë¨¼?€ ê³„ì‚°?˜ê¸° ?„í•¨
	//		if (indexA < indexB)
	//		{
	//			int temp = indexA;
	//			indexA = indexB;
	//			indexB = temp;
	//		}

	//		//?ˆì™¸ì²˜ë¦¬ 
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

	// ?”ë©´ ?ì—­ ?´ì— ?ˆëŠ”ì§€ ì²´í¬?˜ëŠ” ?¨ìˆ˜
	bool IsInRenderArea(const FVector& renderedLocation, float renderedRadius, 
						float minX = -2.0f, float maxX = 2.0f, 
						float minY = -2.0f, float maxY = 2.0f) const
	{
		return (renderedLocation.x + renderedRadius >= minX && 
				renderedLocation.x - renderedRadius <= maxX &&
				renderedLocation.y + renderedRadius >= minY && 
				renderedLocation.y - renderedRadius <= maxY);
	}

	// ë³´ì´??ê°ì²´?€ ë³´ì´ì§€ ?ŠëŠ” ê°ì²´ë¥?ë¶„ë¥˜?˜ëŠ” ?¨ìˆ˜
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

	// ë³´ì´ì§€ ?ŠëŠ” ê°ì²´?¤ì„ ?? œ?˜ëŠ” ?¨ìˆ˜
	void RemoveOutsidePrimitives(const std::vector<int>& invisibleIndices)
	{
		// ?¤ì—?œë????? œ (?¸ë±??ë³€??ë°©ì?)
		for (int i = invisibleIndices.size() - 1; i >= 0; i--)
		{
			int deleteIndex = invisibleIndices[i];
			if (deleteIndex < 0 || deleteIndex >= Size) continue; // ?ˆì „??ì²´í¬
			
			delete primitives[deleteIndex];
			
			// ë§ˆì?ë§??”ì†Œë¥??? œ???„ì¹˜ë¡??´ë™
			primitives[deleteIndex] = primitives[Size - 1];
			primitives[Size - 1] = nullptr;
			Size--;
		}
	}

public:

	UPrimitive** primitives = nullptr;
	int Capacity = 0;
	int Size = 0;

	Merge mergeList[1024] = { -1, -1 };// 1ì¶©ëŒ??2ê°œì˜ êµ¬ê? ?„ìš”?˜ë‹¤ê³??˜ë©´, 1024ë©??‰ë„‰??ê²ƒìœ¼ë¡??ˆìƒ, ë§Œì•½ 1024ë²ˆì„ ?˜ê¸°ë©?ê·¸ëƒ¥ ì¶©ëŒë¡?ì²˜ë¦¬
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

	// À©µµ¿ì Å¬·¡½º ÀÌ¸§ 
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ?ˆë„???€?´í?ë°??´ë¦„
	WCHAR Title[] = L"Game Tech Lab";

	// ê°ì¢… ë©”ì„¸ì§€ë¥?ì²˜ë¦¬???¨ìˆ˜??WndProc???¨ìˆ˜ ?¬ì¸?°ë? WindowClass êµ¬ì¡°ì²´ì— ?±ë¡
	WNDCLASSW wndClass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// ?ˆë„???´ë˜???±ë¡
	RegisterClassW(&wndClass);

	// 1024 * 1024 ?¬ê¸°???ˆë„???ì„±
	hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024, nullptr, nullptr, hInstance, nullptr);

	srand((unsigned int)time(NULL));

	//ê°ì¢… ?ì„±/ì´ˆê¸°??ì½”ë“œë¥??¬ê¸°??ì¶”ê?
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

	// ?Œë ˆ?´ì–´ ?ì„± ë°?ì¶”ê?
	UPlayer* player = new UPlayer();
	PrimitiveVector.push_back(player);

	// ì´ˆê¸° ENEMY?€ PREY ?ì„±
	for (int i = 0; i < 10; ++i)
	{
		PrimitiveVector.push_back(new UEnemy());
		PrimitiveVector.push_back(new UPrey());
	}

	//// êµ¬ì— ê´€??Vertex Buffer????ë²ˆë§Œ ?ì„± ???¬ì‚¬??
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
	const double targetFrameTime = 1000.0f / targetFPS; //ëª©í‘œ ?„ë ˆ??

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0f;

	LARGE_INTEGER CreateStartTime, CurrentTime;
	double CreateInterval = rand() % 1000 + 500;
	QueryPerformanceCounter(&CreateStartTime);
	//// ê³??˜ë‚˜ ì¶”ê??˜ê³  ?œì‘
	//FPrimitiveVector PrimitiveVector;
	//UBall* ball = new UBall();
	//PrimitiveVector.push_back(ball);
	UCamera* cam = new UCamera();

	// Dirty ?Œë˜ê·¸ë? ?„í•œ ë²¡í„° - ?Œë”ë§í•  ê°ì²´???¸ë±?¤ë§Œ ?€??
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

		//¸Ş½ÃÁö Å¥¿¡¼­ msg¸¦ ²¨³»¿À°í Å¥¿¡¼­ Á¦°ÅÇÔ  
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

		// 2. ì¹´ë©”?¼ê? ?Œë ˆ?´ì–´ë¥??°ë¼ê°€?„ë¡ ?…ë°?´íŠ¸
		cam->UpdateCamera(PrimitiveVector.Player);


		// Frame Update
		renderer.Prepare();
		renderer.PrepareShader();

		cam->UpdateCamera(PrimitiveVector[0]);

		// Dirty ?Œë˜ê·??…ë°?´íŠ¸ ë°?ë§?ë³´ë” ë°–ì˜ ê°ì²´ ?? œ
		std::vector<int> OutsidePrimitives; // ?? œ??ê°ì²´?¤ì˜ ?¸ë±??

		// FPrimitiveVector???¨ìˆ˜ë¥??¬ìš©?˜ì—¬ ë³´ë” ë¶„ë¥˜
		PrimitiveVector.ClassifyBorder(cam, InsidePrimitives, OutsidePrimitives);

		// ë§?ë³´ë” ë°–ì˜ ê°ì²´???? œ
		PrimitiveVector.RemoveOutsidePrimitives(OutsidePrimitives);

		// ë³´ë” ?ˆì˜ ê°ì²´?¤ë§Œ ?Œë”ë§?
		for (int idx : InsidePrimitives)
		{
			// ?¤í¬ë¦??ì—?œì˜ ì¢Œí‘œ?€ ?¬ê¸° ê³„ì‚°
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

		// ?Œë ˆ?´ì–´ ?ì„±???ìŠ¤?¸ë¡œ ë³´ì—¬ì£¼ê¸°
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

		//// ?Œë©¸???„ìš”??ì½”ë“œ 
		//renderer.ReleaseVertexBuffer(UBall::vertexBufferSphere);
		
		// WinMain?ì„œ ?ì„±??ë²„í¼ ?´ì œ
		renderer.ReleaseVertexBuffer(vertexBufferSphere);
		renderer.ReleaseConstantBuffer();

		renderer.ReleaseShader();
		renderer.Release();

	return 0;
}