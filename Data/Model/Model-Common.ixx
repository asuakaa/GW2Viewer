export module GW2Viewer.Data.Model:Common;
import GW2Viewer.Common;
import GW2Viewer.Services.Graphics;
import GW2Viewer.Utils.Enum;
import GW2Viewer.Utils.Math;
import std;
import <d3d11.h>;
import <DirectXTex.h>;
import <directxtk/SimpleMath.h>;
import <wrl/client.h>;

export
{
using DirectX::BoundingBox;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using Microsoft::WRL::ComPtr;
using GW2Viewer::Services::Graphics;
}

export namespace GW2Viewer::Data::Model
{
struct Camera;
struct Grid;
struct Material;
struct Mesh;
struct Scene;
struct Viewport;

struct SceneObject
{
    explicit SceneObject(Scene* scene, std::string_view name) : m_scene(scene), m_name(name) { }
    virtual ~SceneObject() = default;

    virtual void Update() { }
    virtual void Render() { }
    virtual void Debug() { }

    Scene* GetScene() const { return m_scene; }
    std::string const& GetName() const { return m_name; }
    void SetName(std::string_view name) { m_name = name; }

private:
    Scene* m_scene;
    std::string m_name;
};

struct Vertex
{
    Vector3 Position;
    Vector3 Normal;
    Vector3 Tangent;
    Vector3 Bitangent;
    Vector2 UV;
    Vector4 Color;
};

}
