#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <DirectXMath.h>
#include <memory>

class Graphics
{
public:
	class GraphicsException;
	class DeviceRemovedException;
	Graphics(HWND hWnd, int horizontalPixelCount, int verticalPixelCount, bool vsync = true);
	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;
	void Present();
	void Shade(bool s);
	void Clear(uint8_t colour = 0x3F);
	std::vector<uint8_t> ReleaseBuffer();
	void Draw(int x, int y, uint8_t colour);
	void DrawChar(int x, int y, char c, uint8_t colour = 0x30, uint8_t bg = 0xFF);
	void DrawString(int x, int y, const std::string& s, uint8_t colour = 0x30, uint8_t bg = 0xFF);
	static constexpr float PIXEL_ASPECT = 8.0f / 7.0f;
private:
	struct CBuffer
	{
		DirectX::XMMATRIX projection;
		float halfWidth;
		float halfHeight;
		float brightness;
		uint32_t width;
	};
	std::shared_ptr<CBuffer> cbuffer;
	static void AssertHResult(HRESULT hr, const std::string& errMsg = "A problem occurred in graphics");
	static std::string HrHex(HRESULT hr);
	static const std::string PIXEL_SHADER_SOURCE;
	static const std::string VERTEX_SHADER_SOURCE;
	static const float BACKGROUND_COLOUR[4];
	static const std::array<uint8_t, 8>* GetFont();
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> instanceBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
	std::vector<uint8_t> screenBuffer;
	const HWND hWnd;
	int clientWidth;
	int clientHeight;
	int horizontalPixelCount;
	int verticalPixelCount;
	bool vsync;
};

class Graphics::GraphicsException : std::exception
{
public:
	GraphicsException(const std::string& msg = "A graphics error occurred");
	virtual const char* what() const noexcept override;
private:
	std::string msg;
};

class Graphics::DeviceRemovedException : Graphics::GraphicsException
{
public:
	DeviceRemovedException(const std::string& msg = "The graphics device was removed");
};
