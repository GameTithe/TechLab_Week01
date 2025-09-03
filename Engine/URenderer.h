#pragma once


// D3D libraries
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")	

// D3D headers
#include <d3d11.h>
#include <d3dcompiler.h>	

#include "VertexInfo.h"
#include "FVector.h"
#include "PlayerData.h"
#include "UIInfo.h"

class URenderer
{
public:

	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;
	ID3D11RasterizerState* RasterizerState = nullptr;
	ID3D11Buffer* ConstantBuffer = nullptr;

	ID3D11Buffer* ConstantUnitBuffer = nullptr;

	//UI
	ID3D11VertexShader* UIVS = nullptr;
	ID3D11PixelShader* UIPS = nullptr;
	ID3D11InputLayout* UIInputLayout = nullptr;
	ID3D11Buffer* UIVertexBuffer = nullptr;
	ID3D11Buffer* UIPerFrameCB = nullptr;

	ID3D11ShaderResourceView* UITitleSRV = nullptr;
	ID3D11ShaderResourceView* UIStartSRV = nullptr;
	ID3D11ShaderResourceView* UIExitSRV = nullptr;
	ID3D11ShaderResourceView* UIMenuSRV = nullptr;
	ID3D11ShaderResourceView* UIVictorySRV = nullptr;
	ID3D11ShaderResourceView* UIGameOverSRV = nullptr;
	ID3D11ShaderResourceView* UINameSRV = nullptr;

	ID3D11SamplerState* UISampler = nullptr;
	ID3D11BlendState* UIAlphaBlend = nullptr;

	FLOAT ClearColor[4] = { .3f, .3f, .3f, 1.0f };
	D3D11_VIEWPORT ViewportInfo;

	// Unit Texture 
	ID3D11ShaderResourceView* WaterBallNoiseSRV = nullptr;
	ID3D11SamplerState* UnitNoiseSampler = nullptr;



public:

	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);

		CreateFrameBuffer();

		CreateRasterizerState();

	}


	void CreateDeviceAndSwapChain(HWND hWindow);

	void ReleaseDeviceAndSwapChain();

	void CreateFrameBuffer();

	void ReleaseFrameBuffer();

	void CreateRasterizerState();

	void ReleaseRasterizerState();

	void Release();

	void SwapBuffer();

	void CreateUIResources();
	 
	void ReleaseUIResource();
	 
	HRESULT LoadTextureWIC(const wchar_t* path, ID3D11ShaderResourceView** outSRV);

	//Shader
public:
	ID3D11VertexShader* SimpleVertexShader;
	ID3D11PixelShader* SimplePixelShader;
	ID3D11InputLayout* SimpleInputLayout;
	unsigned int Stride;

	void CreateShader();

	void CreateUnitShader();


	void ReleaseShader();

public:
	void Prepare();
	

	void PrepareShader();

	void PrepareUnitShader();

	void PrepareShaderUI(ID3D11ShaderResourceView* UISRV);

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);

	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);

	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer);

public:
	struct FConstant
	{
		FVector Offset;
		float Scale;
	};
	void CreateConstantBuffer();

	void ReleaseConstantBuffer();

	void UpdateConstant(FVector Offset, float Scale);

	void UpdateUnitConstant(FVector velocity, int attribute, float time, FVector Offset, float Scale);

	void UpdateUIConstant(float winSize[2], float targetSize[2], bool isHovering, float ratio[2]);
};