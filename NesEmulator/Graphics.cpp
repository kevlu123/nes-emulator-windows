#include "Graphics.h"
#include <cmath>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
using namespace Microsoft::WRL;

const std::string Graphics::VERTEX_SHADER_SOURCE =
R"(
static const float4 COLOUR_SET[64] = 
{
	float4(0.3294117647058823f, 0.3294117647058823f, 0.3294117647058823f, 1.0f),
	float4(0.0000000000000000f, 0.1176470588235294f, 0.4549019607843137f, 1.0f),
	float4(0.0313725490196078f, 0.0627450980392156f, 0.5647058823529412f, 1.0f),
	float4(0.1882352941176470f, 0.0000000000000000f, 0.5333333333333333f, 1.0f),
	float4(0.2666666666666666f, 0.0000000000000000f, 0.3921568627450980f, 1.0f),
	float4(0.3607843137254902f, 0.0000000000000000f, 0.1882352941176470f, 1.0f),
	float4(0.3294117647058823f, 0.0156862745098039f, 0.0000000000000000f, 1.0f),
	float4(0.2352941176470588f, 0.0941176470588235f, 0.0000000000000000f, 1.0f),
	float4(0.1254901960784313f, 0.1647058823529411f, 0.0000000000000000f, 1.0f),
	float4(0.0313725490196078f, 0.2274509803921568f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.2509803921568627f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.2352941176470588f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.1960784313725490f, 0.2352941176470588f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.5960784313725490f, 0.5882352941176471f, 0.5960784313725490f, 1.0f),
	float4(0.0313725490196078f, 0.2980392156862745f, 0.7686274509803922f, 1.0f),
	float4(0.1882352941176470f, 0.1960784313725490f, 0.9254901960784314f, 1.0f),
	float4(0.3607843137254902f, 0.1176470588235294f, 0.8941176470588236f, 1.0f),
	float4(0.5333333333333333f, 0.0784313725490196f, 0.6901960784313725f, 1.0f),
	float4(0.6274509803921569f, 0.0784313725490196f, 0.3921568627450980f, 1.0f),
	float4(0.5960784313725490f, 0.1333333333333333f, 0.1254901960784313f, 1.0f),
	float4(0.4705882352941176f, 0.2352941176470588f, 0.0000000000000000f, 1.0f),
	float4(0.3294117647058823f, 0.3529411764705882f, 0.0000000000000000f, 1.0f),
	float4(0.1568627450980392f, 0.4470588235294118f, 0.0000000000000000f, 1.0f),
	float4(0.0313725490196078f, 0.4862745098039215f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.4627450980392157f, 0.1568627450980392f, 1.0f),
	float4(0.0000000000000000f, 0.4000000000000000f, 0.4705882352941176f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.9254901960784314f, 0.9333333333333333f, 0.9254901960784314f, 1.0f),
	float4(0.2980392156862745f, 0.6039215686274509f, 0.9254901960784314f, 1.0f),
	float4(0.4705882352941176f, 0.4862745098039215f, 0.9254901960784310f, 1.0f),
	float4(0.6901960784313725f, 0.3843137254901961f, 0.9254901960784314f, 1.0f),
	float4(0.8941176470588236f, 0.3294117647058823f, 0.9254901960784310f, 1.0f),
	float4(0.9254901960784314f, 0.3450980392156862f, 0.7058823529411760f, 1.0f),
	float4(0.9254901960784314f, 0.4156862745098039f, 0.3921568627450980f, 1.0f),
	float4(0.8313725490196079f, 0.5333333333333333f, 0.1254901960784313f, 1.0f),
	float4(0.6274509803921569f, 0.6666666666666666f, 0.0000000000000000f, 1.0f),
	float4(0.4549019607843137f, 0.7686274509803922f, 0.0000000000000000f, 1.0f),
	float4(0.2980392156862745f, 0.8156862745098039f, 0.1254901960784313f, 1.0f),
	float4(0.2196078431372549f, 0.8000000000000000f, 0.4235294117647059f, 1.0f),
	float4(0.2196078431372549f, 0.7058823529411765f, 0.8000000000000000f, 1.0f),
	float4(0.2352941176470588f, 0.2352941176470588f, 0.2352941176470588f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.9254901960784314f, 0.9333333333333333f, 0.9254901960784314f, 1.0f),
	float4(0.6588235294117647f, 0.8000000000000000f, 0.9254901960784314f, 1.0f),
	float4(0.7372549019607844f, 0.7372549019607844f, 0.9254901960784314f, 1.0f),
	float4(0.8313725490196079f, 0.6980392156862745f, 0.9254901960784314f, 1.0f),
	float4(0.9254901960784314f, 0.6823529411764706f, 0.9254901960784314f, 1.0f),
	float4(0.9254901960784314f, 0.6823529411764706f, 0.8313725490196079f, 1.0f),
	float4(0.9254901960784314f, 0.7058823529411765f, 0.6901960784313725f, 1.0f),
	float4(0.8941176470588236f, 0.7686274509803922f, 0.5647058823529412f, 1.0f),
	float4(0.8000000000000000f, 0.8235294117647058f, 0.4705882352941176f, 1.0f),
	float4(0.7058823529411765f, 0.8705882352941177f, 0.4705882352941176f, 1.0f),
	float4(0.6588235294117647f, 0.8862745098039215f, 0.5647058823529412f, 1.0f),
	float4(0.5960784313725490f, 0.8862745098039215f, 0.7058823529411765f, 1.0f),
	float4(0.6274509803921569f, 0.8392156862745098f, 0.8941176470588236f, 1.0f),
	float4(0.6274509803921569f, 0.6352941176470588f, 0.6274509803921569f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
	float4(0.0000000000000000f, 0.0000000000000000f, 0.0000000000000000f, 1.0f),
};

cbuffer CBuffer
{
	matrix projection;
	float halfWidth;
	float halfHeight;
	float brightness;
	uint width;
};

struct Output
{
	float4 colour : Colour;
	float4 pos : SV_Position;
};

Output main(float4 localPos : LocalPosition, uint colour : Colour, uint id : SV_InstanceID)
{
	Output output;
	output.pos = localPos + float4(
		id % width - halfWidth,
		-(id / width - halfHeight) - 1,
		0.0f,
		0.0f
	);
	output.pos = mul(projection, output.pos);
	output.colour = brightness * COLOUR_SET[colour % 64];
	return output;
}
)";

const std::string Graphics::PIXEL_SHADER_SOURCE =
R"(
float4 main(float4 colour : Colour) : SV_Target
{
	return colour;
}
)";

const float Graphics::BACKGROUND_COLOUR[4] { 0.0f, 0.0f, 0.0f, 1.0f };

Graphics::GraphicsException::GraphicsException(const std::string& msg) :
	msg(msg)
{
}

const char* Graphics::GraphicsException::what() const noexcept
{
	return this->msg.c_str();
}

Graphics::DeviceRemovedException::DeviceRemovedException(const std::string& msg) :
	GraphicsException(msg)
{
}

Graphics::Graphics(HWND hWnd, int horizontalPixelCount, int verticalPixelCount, bool vsync) :
	hWnd(hWnd),
	horizontalPixelCount(horizontalPixelCount),
	verticalPixelCount(verticalPixelCount),
	vsync(vsync)
{
	// Get client size
	RECT rc;
	GetClientRect(hWnd, &rc);
	this->clientWidth = rc.right;
	this->clientHeight = rc.bottom;

	// Create and bind device, context, and swap chian
	DXGI_SWAP_CHAIN_DESC scDesc{};
	scDesc.BufferCount = 2;
	scDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scDesc.BufferDesc.Width = this->clientWidth;
	scDesc.BufferDesc.Height = this->clientHeight;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow = hWnd;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Windowed = true;
	scDesc.Flags = NULL;
	AssertHResult(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
#ifdef _DEBUG
		D3D11_CREATE_DEVICE_DEBUG,
#else
		NULL,
#endif
		nullptr,
		NULL,
		D3D11_SDK_VERSION,
		&scDesc,
		&this->swapChain,
		&this->device,
		nullptr,
		&this->context
	), "Failed to create device, swap chain, and context");

	// Disable alt-enter
	ComPtr<IDXGIDevice> dxgiDevice;
	AssertHResult(this->device->QueryInterface(
		__uuidof(IDXGIDevice),
		&dxgiDevice
	), "Failed to get dxgi device");
	ComPtr<IDXGIAdapter> adapter;
	AssertHResult(dxgiDevice->GetParent(
		__uuidof(IDXGIAdapter),
		&adapter
	), "Failed to get dxgi adapter");
	ComPtr<IDXGIFactory> factory;
	AssertHResult(adapter->GetParent(
		__uuidof(IDXGIFactory),
		&factory
	), "Failed to get dxgi factory");
	AssertHResult(factory->MakeWindowAssociation(
		hWnd,
		DXGI_MWA_NO_ALT_ENTER
	), "Failed to disable alt-enter");

	// Create and bind render target view
	ComPtr<ID3D11Texture2D> backBuffer;
	AssertHResult(this->swapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		&backBuffer
	), "Failed to get back buffer of swap chain");
	AssertHResult(this->device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&this->renderTargetView
	), "Failed to create render target view");
	this->context->OMSetRenderTargets(
		1,
		this->renderTargetView.GetAddressOf(),
		nullptr
	);

	// Compile, create, and bind pixel shader
	ComPtr<ID3DBlob> psBlob;
	AssertHResult(D3DCompile(
		PIXEL_SHADER_SOURCE.c_str(),
		PIXEL_SHADER_SOURCE.size(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
#ifdef _DEBUG
		D3DCOMPILE_DEBUG |
#else
		D3DCOMPILE_OPTIMIZATION_LEVEL3 |
#endif
		D3DCOMPILE_ENABLE_STRICTNESS,
		NULL,
		&psBlob,
		nullptr
	), "Failed to compile pixel shader");
	AssertHResult(this->device->CreatePixelShader(
		psBlob->GetBufferPointer(),
		psBlob->GetBufferSize(),
		nullptr,
		&this->pixelShader
	), "Failed to create pixel shader");
	this->context->PSSetShader(
		this->pixelShader.Get(),
		nullptr,
		0
	);

	// Compile, create, and bind vertex shader
	ComPtr<ID3DBlob> vsBlob;
	AssertHResult(D3DCompile(
		VERTEX_SHADER_SOURCE.c_str(),
		VERTEX_SHADER_SOURCE.size(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
#ifdef _DEBUG
		D3DCOMPILE_DEBUG |
#else
		D3DCOMPILE_OPTIMIZATION_LEVEL3 |
#endif
		D3DCOMPILE_ENABLE_STRICTNESS,
		NULL,
		&vsBlob,
		nullptr
	), "Failed to compile vertex shader");
	AssertHResult(this->device->CreateVertexShader(
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		nullptr,
		&this->vertexShader
	), "Failed to create vertex shader");
	this->context->VSSetShader(
		this->vertexShader.Get(),
		nullptr,
		0
	);

	// Create and bind input layout
	D3D11_INPUT_ELEMENT_DESC ilDesc[]
	{
		{ "LocalPosition", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Colour", 0, DXGI_FORMAT_R8_UINT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};
	AssertHResult(this->device->CreateInputLayout(
		ilDesc,
		static_cast<UINT>(std::size(ilDesc)),
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		&this->inputLayout
	), "Failed to create input layout");
	this->context->IASetInputLayout(this->inputLayout.Get());

	// Set viewport
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(this->clientWidth);
	vp.Height = static_cast<float>(this->clientHeight);
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	this->context->RSSetViewports(1, &vp);

	// Set topology
	this->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create vertex buffer
	struct { float x, y, z, w; } vertices[6]
	{
		{ 0.0f, 0.0f, 0.5f, 1.0f },
		{ 0.0f, 1.0f, 0.5f, 1.0f },
		{ 1.0f, 0.0f, 0.5f, 1.0f },
		{ 1.0f, 0.0f, 0.5f, 1.0f },
		{ 0.0f, 1.0f, 0.5f, 1.0f },
		{ 1.0f, 1.0f, 0.5f, 1.0f },
	};
	D3D11_SUBRESOURCE_DATA vbData{};
	vbData.pSysMem = vertices;
	D3D11_BUFFER_DESC vbDesc{};
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.ByteWidth = 4 * sizeof(float) * 6;
	vbDesc.CPUAccessFlags = NULL;
	vbDesc.StructureByteStride = 4 * sizeof(float);
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	AssertHResult(this->device->CreateBuffer(
		&vbDesc,
		&vbData,
		&this->vertexBuffer
	), "Failed to create vertex buffer");

	// Create instance buffer
	this->screenBuffer.resize(this->horizontalPixelCount * this->verticalPixelCount, 0x3F);
	D3D11_SUBRESOURCE_DATA ibData{};
	ibData.pSysMem = this->screenBuffer.data();
	D3D11_BUFFER_DESC ibDesc{};
	ibDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint8_t) * this->screenBuffer.size());
	ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ibDesc.StructureByteStride = sizeof(uint8_t);
	ibDesc.Usage = D3D11_USAGE_DYNAMIC;
	AssertHResult(this->device->CreateBuffer(
		&ibDesc,
		&ibData,
		&this->instanceBuffer
	), "Failed to create instance buffer");

	// Bind vertex and instance buffer
	ID3D11Buffer* buffers[2]
	{
		this->vertexBuffer.Get(),
		this->instanceBuffer.Get()
	};
	const UINT stride[2] = { 4 * sizeof(float), sizeof(uint8_t) };
	const UINT offset[2] = { 0, 0 };
	this->context->IASetVertexBuffers(
		0,
		2,
		buffers,
		stride,
		offset
	);

	// Create and bind constant buffer
	CBuffer* mem = (CBuffer*)_aligned_malloc(sizeof(CBuffer), 16);
	if (!mem)
		throw std::bad_alloc();
	this->cbuffer.reset(mem, _aligned_free);
	if (this->clientWidth / static_cast<float>(horizontalPixelCount) >= PIXEL_ASPECT * this->clientHeight / static_cast<float>(verticalPixelCount))
	{
		this->cbuffer->projection = DirectX::XMMatrixOrthographicLH(
			static_cast<float>(this->clientWidth) / this->clientHeight * verticalPixelCount / PIXEL_ASPECT,
			static_cast<float>(verticalPixelCount),
			0.1f,
			1.0f
		);
	}
	else
	{
		this->cbuffer->projection = DirectX::XMMatrixOrthographicLH(
			static_cast<float>(horizontalPixelCount),
			static_cast<float>(this->clientHeight) / this->clientWidth * horizontalPixelCount * PIXEL_ASPECT,
			0.1f,
			1.0f
		);
	}
	this->cbuffer->halfWidth = horizontalPixelCount / 2.0f;
	this->cbuffer->halfHeight = verticalPixelCount / 2.0f;
	this->cbuffer->brightness = 1.0f;
	this->cbuffer->width = horizontalPixelCount;
	D3D11_SUBRESOURCE_DATA cbData{};
	cbData.pSysMem = this->cbuffer.get();
	D3D11_BUFFER_DESC cbDesc{};
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = sizeof(CBuffer);
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	AssertHResult(this->device->CreateBuffer(
		&cbDesc,
		&cbData,
		&this->constantBuffer
	), "Failed to create constant buffer");
	this->context->VSSetConstantBuffers(
		0,
		1,
		this->constantBuffer.GetAddressOf()
	);
}

std::vector<uint8_t> Graphics::ReleaseBuffer()
{
	return std::move(this->screenBuffer);
}

std::string Graphics::HrHex(HRESULT hr)
{
	std::string s;
	for (size_t i = 0; i < sizeof(HRESULT) * 2; i++)
	{
		s = "0123456789ABCDEF"[hr & 0xF] + s;
		hr >>= 4;
	}
	return "0x" + s;
}

void Graphics::AssertHResult(HRESULT hr, const std::string& errMsg)
{
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
		throw DeviceRemovedException("Device removed.\n(Code " + HrHex(hr) + ")");
	if (FAILED(hr))
		throw GraphicsException(errMsg + ".\n(Code " + HrHex(hr) + ")");
}

void Graphics::Draw(int x, int y, uint8_t colour)
{
	if (x >= 0 && x < this->horizontalPixelCount && y >= 0 && y < this->verticalPixelCount)
		this->screenBuffer[y * this->horizontalPixelCount + x] = colour;
#ifdef _DEBUG
	else
		throw GraphicsException("Coordinate out of range");
#endif
}

void Graphics::Present()
{
	if (!this->context)
		return;

	// Update instance buffer
	D3D11_MAPPED_SUBRESOURCE map;
	AssertHResult(this->context->Map(
		this->instanceBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		NULL,
		&map
	), "Failed to map instance buffer");
	std::memcpy(
		map.pData,
		this->screenBuffer.data(),
		this->screenBuffer.size() * sizeof(uint8_t)
	);
	this->context->Unmap(
		this->instanceBuffer.Get(),
		0
	);

	// Draw and present
	this->context->DrawInstanced(
		6,
		static_cast<UINT>(this->screenBuffer.size()),
		0,
		0
	);
	AssertHResult(this->swapChain->Present(
		this->vsync,
		NULL
	), "Errors while presenting swap chain");
	this->context->OMSetRenderTargets(
		1,
		this->renderTargetView.GetAddressOf(),
		nullptr
	);
	this->context->ClearRenderTargetView(
		this->renderTargetView.Get(),
		Graphics::BACKGROUND_COLOUR
	);
}

void Graphics::Clear(uint8_t colour)
{
	std::memset(
		this->screenBuffer.data(),
		colour,
		this->screenBuffer.size()
	);
}

const std::array<uint8_t, 8>* Graphics::GetFont()
{
	static std::vector<std::array<uint8_t, 8>> fontsMemory(256);
#if CHAR_MIN < 0
	static std::array<uint8_t, 8>* fonts = [&] {
		std::array<uint8_t, 8>* fonts = fontsMemory.data() + 128;
#else
	static std::array<uint8_t, 8>* fonts = [&] {
		std::array<uint8_t, 8>* fonts = fontsMemory.data();
#endif

		for (int i = 0; i < 256; i++)
			fonts[CHAR_MIN + i] = { 0xFE, 0x82, 0xAA, 0x82, 0xBA, 0x82, 0xFE, 0x00, };
		fonts[' '] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
		fonts['0'] = { 0x7C, 0xC6, 0xCE, 0xFE, 0xE6, 0xC6, 0x7C, 0x00, };
		fonts['1'] = { 0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0xFE, 0x00, };
		fonts['2'] = { 0x7C, 0xC6, 0x06, 0x0C, 0x38, 0x60, 0xFE, 0x00, };
		fonts['3'] = { 0x7C, 0xC6, 0x06, 0x7E, 0x06, 0xC6, 0x7C, 0x00, };
		fonts['4'] = { 0x04, 0x0C, 0x1C, 0x34, 0x64, 0xFE, 0x04, 0x00, };
		fonts['5'] = { 0xFE, 0xC0, 0xC0, 0xFC, 0x0E, 0x0E, 0xFC, 0x00, };
		fonts['6'] = { 0x3E, 0x70, 0xE0, 0xFC, 0xE6, 0xE6, 0x7C, 0x00, };
		fonts['7'] = { 0xFE, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00, };
		fonts['8'] = { 0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00, };
		fonts['9'] = { 0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x1C, 0x78, 0x00, };
		fonts['A'] = { 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0x00, };
		fonts['B'] = { 0xFC, 0xC6, 0xC6, 0xFC, 0xC6, 0xC6, 0xFC, 0x00, };
		fonts['C'] = { 0x7C, 0xE6, 0xC0, 0xC0, 0xC0, 0xE6, 0x7C, 0x00, };
		fonts['D'] = { 0xFC, 0xC6, 0xC2, 0xC2, 0xC2, 0xC6, 0xFC, 0x00, };
		fonts['E'] = { 0xFE, 0xC0, 0xC0, 0xF8, 0xC0, 0xC0, 0xFE, 0x00, };
		fonts['F'] = { 0xFE, 0xC0, 0xC0, 0xF8, 0xC0, 0xC0, 0xC0, 0x00, };
		fonts['G'] = { 0x7C, 0xE6, 0xC0, 0xDE, 0xC2, 0xE6, 0x7C, 0x00, };
		fonts['H'] = { 0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00, };
		fonts['I'] = { 0xFE, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFE, 0x00, };
		fonts['J'] = { 0xFE, 0x18, 0x18, 0x18, 0x98, 0xF8, 0x70, 0x00, };
		fonts['K'] = { 0xC6, 0xCC, 0xF8, 0xF0, 0xD8, 0xCC, 0xC6, 0x00, };
		fonts['L'] = { 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFE, 0x00, };
		fonts['M'] = { 0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00, };
		fonts['N'] = { 0xC6, 0xE6, 0xF6, 0xFE, 0xDE, 0xCE, 0xC6, 0x00, };
		fonts['O'] = { 0x7C, 0xEE, 0xC6, 0xC6, 0xC6, 0xEE, 0x7C, 0x00, };
		fonts['P'] = { 0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0, 0xC0, 0x00, };
		fonts['Q'] = { 0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0xFC, 0x1E, 0x00, };
		fonts['R'] = { 0xFC, 0xC6, 0xC6, 0xFC, 0xF8, 0xCC, 0xC6, 0x00, };
		fonts['S'] = { 0x7C, 0xC6, 0xC0, 0x7C, 0x06, 0xC6, 0x7C, 0x00, };
		fonts['T'] = { 0xFE, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, };
		fonts['U'] = { 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xEE, 0x7C, 0x00, };
		fonts['V'] = { 0xC6, 0xC6, 0xC6, 0xEE, 0x6C, 0x7C, 0x18, 0x00, };
		fonts['W'] = { 0xC6, 0xC6, 0xC6, 0xD6, 0xD6, 0xFE, 0x6C, 0x00, };
		fonts['X'] = { 0xC6, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0xC6, 0x00, };
		fonts['Y'] = { 0xC6, 0xC6, 0x7C, 0x18, 0x18, 0x18, 0x18, 0x00, };
		fonts['Z'] = { 0xFE, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFE, 0x00, };
		fonts['a'] = { 0x00, 0x00, 0x7E, 0xC6, 0xC6, 0xCE, 0x7A, 0x00, };
		fonts['b'] = { 0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xFC, 0x00, };
		fonts['c'] = { 0x00, 0x00, 0x7C, 0xC2, 0xC0, 0xC2, 0x7C, 0x00, };
		fonts['d'] = { 0x06, 0x06, 0x7E, 0xC6, 0xC6, 0xC6, 0x7E, 0x00, };
		fonts['e'] = { 0x00, 0x00, 0x7C, 0xC2, 0xFE, 0xC0, 0x7E, 0x00, };
		fonts['f'] = { 0x3C, 0x60, 0x60, 0xF8, 0x60, 0x60, 0x60, 0x00, };
		fonts['g'] = { 0x00, 0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x7C, 0x00, };
		fonts['h'] = { 0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, };
		fonts['i'] = { 0x00, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, };
		fonts['j'] = { 0x00, 0x18, 0x00, 0x18, 0x18, 0x18, 0x30, 0x00, };
		fonts['k'] = { 0xC0, 0xFC, 0xC6, 0xC6, 0xFC, 0xD8, 0xCE, 0x00, };
		fonts['l'] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x00, };
		fonts['m'] = { 0x00, 0x00, 0x7C, 0xFE, 0xD6, 0xD6, 0xD6, 0x00, };
		fonts['n'] = { 0x00, 0x00, 0xDC, 0xE6, 0xC6, 0xC6, 0xC6, 0x00, };
		fonts['o'] = { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, };
		fonts['p'] = { 0x00, 0x00, 0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0x00, };
		fonts['q'] = { 0x00, 0x00, 0x7C, 0xCC, 0xCC, 0x7C, 0x0E, 0x00, };
		fonts['r'] = { 0x00, 0x00, 0xD8, 0xE4, 0xC0, 0xC0, 0xC0, 0x00, };
		fonts['s'] = { 0x00, 0x00, 0x7C, 0xE0, 0x7C, 0x0E, 0x7C, 0x00, };
		fonts['t'] = { 0x18, 0x18, 0xFE, 0x18, 0x18, 0x18, 0x18, 0x00, };
		fonts['u'] = { 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xFE, 0x7E, 0x00, };
		fonts['v'] = { 0x00, 0x00, 0xC6, 0xC6, 0xEE, 0x6C, 0x38, 0x00, };
		fonts['w'] = { 0x00, 0x00, 0xC6, 0xC6, 0xD6, 0xFE, 0x6C, 0x00, };
		fonts['x'] = { 0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00, };
		fonts['y'] = { 0x00, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C, 0x00, };
		fonts['z'] = { 0x00, 0x00, 0xFE, 0x0C, 0x38, 0x60, 0xFE, 0x00, };
		fonts['_'] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, };
		fonts['!'] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00, };
		fonts['@'] = { 0x7C, 0x82, 0xBA, 0xAA, 0xBE, 0x80, 0x7C, 0x00, };
		fonts['#'] = { 0x44, 0xFE, 0x44, 0x44, 0x44, 0xFE, 0x44, 0x00, };
		fonts['$'] = { 0x10, 0x7C, 0x90, 0x7C, 0x12, 0x7C, 0x10, 0x00, };
		fonts['%'] = { 0x42, 0xA4, 0x48, 0x10, 0x24, 0x4A, 0x84, 0x00, };
		fonts['^'] = { 0x00, 0x10, 0x28, 0x44, 0x82, 0x00, 0x00, 0x00, };
		fonts['&'] = { 0x70, 0x88, 0x68, 0x12, 0x6A, 0x84, 0x7A, 0x00, };
		fonts['*'] = { 0x10, 0x92, 0x54, 0x38, 0x54, 0x92, 0x10, 0x00, };
		fonts['['] = { 0x7C, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7C, 0x00, };
		fonts[']'] = { 0x7C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x7C, 0x00, };
		fonts['('] = { 0x1C, 0x20, 0x40, 0x40, 0x40, 0x20, 0x1C, 0x00, };
		fonts[')'] = { 0x70, 0x08, 0x04, 0x04, 0x04, 0x08, 0x70, 0x00, };
		fonts['{'] = { 0x18, 0x20, 0x40, 0x20, 0x40, 0x20, 0x18, 0x00, };
		fonts['}'] = { 0x30, 0x08, 0x04, 0x08, 0x04, 0x08, 0x30, 0x00, };
		fonts[','] = { 0x00, 0x00, 0x00, 0x00, 0x18, 0x08, 0x10, 0x00, };
		fonts['?'] = { 0x38, 0x44, 0x04, 0x08, 0x10, 0x00, 0x10, 0x00, };
		fonts['-'] = { 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, };
		fonts['+'] = { 0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x00, 0x00, };
		fonts['='] = { 0x00, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x00, 0x00, };
		fonts['|'] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, };
		fonts['/'] = { 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, };
		fonts['\\'] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, };
		fonts['\''] = { 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, };
		fonts['"'] = { 0x00, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, };
		fonts[':'] = { 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, };
		fonts[';'] = { 0x00, 0x18, 0x18, 0x00, 0x18, 0x08, 0x10, 0x00, };
		fonts['<'] = { 0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08, 0x00, };
		fonts['>'] = { 0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20, 0x00, };
		fonts['.'] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, };
		fonts['~'] = { 0x00, 0x00, 0x60, 0x92, 0x0C, 0x00, 0x00, 0x00, };
		fonts['`'] = { 0x00, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, };
		fonts['\x1'] = { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, };
		return fonts;
	}();
	return fonts;
}

void Graphics::DrawChar(int x, int y, char c, uint8_t colour, uint8_t bg)
{
	for (int dy = 0; dy < 8; dy++)
		for (int dx = 0; dx < 8; dx++)
			if (bg == 0xFF)
			{
				if (Graphics::GetFont()[c][dy] & (0x80 >> dx))
					this->Draw(x + dx, y + dy, colour);
			}
			else
			{
				this->Draw(x + dx, y + dy, (Graphics::GetFont()[c][dy] & (0x80 >> dx)) ? colour : bg);
			}
}

void Graphics::DrawString(int x, int y, const std::string& s, uint8_t colour, uint8_t bg)
{
	for (int i = 0; i < (int)s.size() && x + 8 * i < this->horizontalPixelCount; i++)
		this->DrawChar(x + 8 * i, y, s[i], colour, bg);
}

void Graphics::Shade(bool f)
{
	if (!this->context)
		return;

	// Update constant buffer
	this->cbuffer->brightness = f ? 1.0f : 0.3f;
	D3D11_MAPPED_SUBRESOURCE map{};
	AssertHResult(this->context->Map(
		this->constantBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		NULL,
		&map
	), "Failed to map constant buffer");

	memcpy(map.pData, this->cbuffer.get(), sizeof(CBuffer));

	this->context->Unmap(this->constantBuffer.Get(), 0);
}

