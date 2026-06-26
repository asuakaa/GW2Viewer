export module GW2Viewer.Services.Graphics;
import GW2Viewer.Common;
import GW2Viewer.UI.ImGui;
import std;
import <d3d11.h>;
import <wrl/client.h>;

export namespace GW2Viewer::Services
{

struct Graphics
{
    ImVec4 BackgroundColor { };
    bool VSync = true;

    ID3D11Device* Device = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    ID3D11DeviceContext* Context = nullptr;
    ID3D11RenderTargetView* RenderTargetView = nullptr;

    bool Start(HWND outputWindow);
    void Stop();

    bool Resize(ImVec2i size);
    void Clear() const;
    void Present() const;

    struct Buffer
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> Ptr;
        uint32 Size = 0;
        uint32 Count = 0;

        void Update(auto&& data);
    };
    Buffer CreateBuffer(D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, std::ranges::contiguous_range auto&& data);
    Buffer CreateIndexBuffer(std::ranges::contiguous_range auto&& data) { return CreateBuffer(D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, data); }
    Buffer CreateVertexBuffer(std::ranges::contiguous_range auto&& data) { return CreateBuffer(D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, data); }
    Buffer CreateConstantBuffer(auto&& data) { return CreateBuffer(D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, std::span { &data, 1 }); }

    struct RenderTexture
    {
        void Initialize(ImVec2i size);
        void Bind();
        void Clear(ImVec4 color) const;

        auto GetSize() const { return m_size; }
        auto GetAspectRatio() const { return (float)m_size.x / m_size.y; }
        auto GetShaderResourceView() const { return m_shaderResourceView.Get(); }

    private:
        ImVec2i m_size;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    };

private:
    bool CreateRenderTargetView();

    void Release(auto& field)
    {
        if (auto const obj = std::exchange(field, nullptr))
        {
            try { obj->Release(); }
            catch (...) { }
        }
    }
};

}

export namespace GW2Viewer::G::Services { GW2Viewer::Services::Graphics Graphics; }

export namespace GW2Viewer::Services
{

void Graphics::Buffer::Update(auto&& data)
{
    G::Services::Graphics.Context->UpdateSubresource(Ptr.Get(), 0, nullptr, &data, 0, 0);
}

Graphics::Buffer Graphics::CreateBuffer(D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, std::ranges::contiguous_range auto&& data)
{
    Buffer result { .Size = (uint32)sizeof(std::ranges::range_value_t<decltype(data)>), .Count = (uint32)data.size() };
    D3D11_BUFFER_DESC vbd
    {
        .ByteWidth = result.Size * result.Count,
        .Usage = usage,
        .BindFlags = (uint32)bindFlags,
    };
    D3D11_SUBRESOURCE_DATA vinitData
    {
        .pSysMem = data.data(),
    };
    Device->CreateBuffer(&vbd, &vinitData, &result.Ptr);
    return result;
}

}

module :private;

namespace GW2Viewer::Services
{

bool Graphics::Start(HWND outputWindow)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = outputWindow;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &SwapChain, &Device, &featureLevel, &Context)))
        return false;

    return CreateRenderTargetView();
}

void Graphics::Stop()
{
    Release(RenderTargetView);
    Release(SwapChain);
    Release(Context);
    Release(Device);
}

bool Graphics::Resize(ImVec2i size)
{
    if (!Device)
        return true;

    Release(RenderTargetView);
    if (FAILED(SwapChain->ResizeBuffers(0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0)))
        return false;
    return CreateRenderTargetView();
}

void Graphics::Clear() const
{
    Context->OMSetRenderTargets(1, &RenderTargetView, nullptr);
    Context->ClearRenderTargetView(RenderTargetView, &BackgroundColor.x);
}

void Graphics::Present() const
{
    SwapChain->Present(VSync, 0);
}

bool Graphics::CreateRenderTargetView()
{
    ID3D11Texture2D* backBuffer;
    if (FAILED(SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
        return false;
    if (FAILED(Device->CreateRenderTargetView(backBuffer, nullptr, &RenderTargetView)))
        return false;
    Release(backBuffer);
    return true;
}

void Graphics::RenderTexture::Initialize(ImVec2i size)
{
    if (m_size.x == size.x && m_size.y == size.y)
        return;
    m_size = size;

    m_renderTargetView.Reset();
    m_shaderResourceView.Reset();
    m_depthStencilView.Reset();

    D3D11_TEXTURE2D_DESC const texDesc
    {
        .Width = (UINT)m_size.x,
        .Height = (UINT)m_size.y,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
    };
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    auto device = G::Services::Graphics.Device;
    if (FAILED(device->CreateTexture2D(&texDesc, nullptr, &texture))) return;
    if (FAILED(device->CreateRenderTargetView(texture.Get(), nullptr, &m_renderTargetView))) return;
    if (FAILED(device->CreateShaderResourceView(texture.Get(), nullptr, &m_shaderResourceView))) return;

    auto depthDesc = texDesc;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTex;
    if (FAILED(device->CreateTexture2D(&depthDesc, nullptr, &depthTex))) return;
    if (FAILED(device->CreateDepthStencilView(depthTex.Get(), nullptr, &m_depthStencilView))) return;
}

void Graphics::RenderTexture::Bind()
{
    D3D11_VIEWPORT const vp = { 0.0f, 0.0f, (float)m_size.x, (float)m_size.y, 0.0f, 1.0f };
    auto context = G::Services::Graphics.Context;
    context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
    context->RSSetViewports(1, &vp);
}

void Graphics::RenderTexture::Clear(ImVec4 color) const
{
    auto context = G::Services::Graphics.Context;
    context->ClearRenderTargetView(m_renderTargetView.Get(), &color.x);
    context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

}
