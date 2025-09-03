#include "URenderer.h"
#include "UIInfo.h"

#include <vector>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")  


void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
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

void URenderer::ReleaseDeviceAndSwapChain()
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

void URenderer::CreateFrameBuffer()
{
	// �?버처 ?�스�?가?�오�?
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

	// RTV ?�성
	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // ?�반?�인 RGBA ?�맷
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D ?�스처로 ?�정	

	Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

void URenderer::ReleaseFrameBuffer()
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

void URenderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID; // 채우�?모드  
	rasterizerDesc.CullMode = D3D11_CULL_BACK; // �??�이??컬링  

	Device->CreateRasterizerState(&rasterizerDesc, &RasterizerState);
}

void URenderer::ReleaseRasterizerState()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
}

void URenderer::Release()
{
	RasterizerState->Release();

	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // ?�더 ?�겟을 초기??


	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
}

void URenderer::SwapBuffer()
{
	SwapChain->Present(1, 0); // 1: VSync ?�성??=> GPU ?�레???�도 & ?�면 갱신 ?�도 ?�기??
}
void URenderer::CreateUIResources()
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
	LoadTextureWIC(L"ui_gameover.png", &UIGameOverSRV);
	LoadTextureWIC(L"ui_name.png", &UINameSRV);
	LoadTextureWIC(L"ui_menu.png", &UIMenuSRV);

}
void URenderer::ReleaseUIResource()
{
	if (UIAlphaBlend) { UIAlphaBlend->Release(); UIAlphaBlend = nullptr; }
	if (UISampler) { UISampler->Release();    UISampler = nullptr; }
	if (UITitleSRV) { UITitleSRV->Release();   UITitleSRV = nullptr; }
	if (UIStartSRV) { UIStartSRV->Release();   UIStartSRV = nullptr; }
	if (UIExitSRV) { UIExitSRV->Release();   UIExitSRV = nullptr; }
	if (UIGameOverSRV) { UIGameOverSRV->Release();   UIGameOverSRV = nullptr; }
	if (UINameSRV) { UINameSRV->Release();   UINameSRV = nullptr; }
	if (UIPerFrameCB) { UIPerFrameCB->Release(); UIPerFrameCB = nullptr; }
	if (UIVertexBuffer) { UIVertexBuffer->Release(); UIVertexBuffer = nullptr; }
	if (UIInputLayout) { UIInputLayout->Release(); UIInputLayout = nullptr; }
	if (UIPS) { UIPS->Release();         UIPS = nullptr; }
	if (UIVS) { UIVS->Release();         UIVS = nullptr; }
}

HRESULT URenderer::LoadTextureWIC(const wchar_t* path, ID3D11ShaderResourceView** outSRV)
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
	if (SUCCEEDED(hr))
	{
		D3D11_TEXTURE2D_DESC td{}; td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		D3D11_SUBRESOURCE_DATA srd{ pixels.data(), (UINT)(w * 4), 0 };
		ID3D11Texture2D* tex = nullptr; hr = Device->CreateTexture2D(&td, &srd, &tex);
		if (SUCCEEDED(hr))
		{
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

void URenderer::CreateShader()
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

	Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

	Stride = sizeof(FVertexSimple);

	vertexShaderCSO->Release();
	pixelShaderCSO->Release();
}

void URenderer::CreateUnitShader()
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
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,       0, 12,  D3D11_INPUT_PER_VERTEX_DATA,0},
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

	Stride = sizeof(FVertexSimple);

	vertexShaderCSO->Release();
	pixelShaderCSO->Release();

	// Sampler
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	Device->CreateSamplerState(&samplerDesc, &UnitNoiseSampler);

	LoadTextureWIC(L"noise.png", &WaterBallNoiseSRV);
}


void URenderer::ReleaseShader()
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


void URenderer::Prepare()
{
	DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Rasterization State
	DeviceContext->RSSetViewports(1, &ViewportInfo);
	DeviceContext->RSSetState(RasterizerState);

	DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
	DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void URenderer::PrepareShader()
{
	DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
	DeviceContext->IASetInputLayout(SimpleInputLayout);

	if (ConstantBuffer)
	{
		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
}

void URenderer::PrepareUnitShader()
{
	DeviceContext->IASetInputLayout(SimpleInputLayout);
	DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);

	if (ConstantBuffer)
	{
		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
	if (ConstantUnitBuffer)
	{
		DeviceContext->VSSetConstantBuffers(1, 1, &ConstantUnitBuffer);
		DeviceContext->PSSetConstantBuffers(1, 1, &ConstantUnitBuffer);
	}

	float blendFactor[4] = { 0,0,0,0 };
	DeviceContext->OMSetBlendState(UIAlphaBlend, blendFactor, 0xffffffff);

	//TODO 3��
	DeviceContext->PSSetShaderResources(0, 1, &WaterBallNoiseSRV);
	DeviceContext->PSSetSamplers(0, 1, &UnitNoiseSampler);

}
void URenderer::PrepareShaderUI(ID3D11ShaderResourceView* UISRV)
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

void URenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
{
	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
	DeviceContext->Draw(numVertices, 0);
}

ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
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

void URenderer::ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
{
	vertexBuffer->Release();
}

void URenderer::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC constantBufferDesc = {};
	constantBufferDesc.ByteWidth = (sizeof(FConstant) + 0xf) & 0xfffffff0; // align 16byte 
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device->CreateBuffer(&constantBufferDesc, nullptr, &ConstantBuffer);


	D3D11_BUFFER_DESC constantUnitBufferDesc = {};
	constantUnitBufferDesc.ByteWidth = (sizeof(FPlayerInfo) + 0xf) & 0xfffffff0; // align 16byte 
	constantUnitBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantUnitBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantUnitBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device->CreateBuffer(&constantUnitBufferDesc, nullptr, &ConstantUnitBuffer);
}

void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
}

void URenderer::UpdateConstant(FVector Offset, float Scale)
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

void URenderer::UpdateUnitConstant(int attribute, float time, FVector Offset, float Scale)
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

	if (ConstantUnitBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE constantUnitBufferMSR;

		DeviceContext->Map(ConstantUnitBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantUnitBufferMSR);
		FPlayerInfo* constants = (FPlayerInfo*)constantUnitBufferMSR.pData;

		//constants->att = player;
		//constants->iTime = time;
		float resolution[2] = { winWidth, winHeight };
		constants->att = attribute;
		memcpy(constants->resolution, resolution, sizeof(float) * 2);
		constants->iTime = time;

		DeviceContext->Unmap(ConstantUnitBuffer, 0);

	}
}
void URenderer::UpdateUIConstant(float winSize[2], float targetSize[2], bool isHovering, float ratio[2])
{
	if (UIPerFrameCB)
	{
		UIInfo mainUI = { {winSize[0], winSize[1]}, isHovering ? 1 : 0, 0.0f };

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