export module GW2Viewer.Data.Model:Material;
import :Common;

export namespace GW2Viewer::Data::Model
{

struct Material
{
    explicit Material(Scene* scene) : m_scene(scene) { }

    void Bind();
    void Debug();

    void CreateBasic();
    void CreateDebugShapes() { CreateBasic(); m_debugShapesMaterial = true; }

private:
    Scene* m_scene;
    bool m_alphaTestTransparency = false;
    bool m_debugShapesMaterial = false;

    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11RasterizerState> m_rasterizerState;
    ComPtr<ID3D11BlendState> m_blendState;
    ComPtr<ID3D11SamplerState> m_samplerState;
    Graphics::Buffer m_constantBuffer;
};

}
