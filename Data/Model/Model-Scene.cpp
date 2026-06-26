module GW2Viewer.Data.Model;
import :Camera;
import :Grid;
import :Material;
import :Mesh;
import :Scene;
#include "Macros.h"

namespace GW2Viewer::Data::Model
{

Scene::Scene(Scene* scene, std::string_view name): SceneObject(scene, name)
{
    (m_basicMaterial = std::make_unique<Material>(this))->CreateBasic();
    (m_debugShapesMaterial = std::make_unique<Material>(this))->CreateDebugShapes();

    D3D11_TEXTURE2D_DESC const desc
    {
        .Width = 1, .Height = 1, .MipLevels = 1, .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc = { 1, 0 },
        .Usage = D3D11_USAGE_IMMUTABLE, .BindFlags = D3D11_BIND_SHADER_RESOURCE
    };
    uint32 const whitePixel = 0xFFFFFFFF;
    D3D11_SUBRESOURCE_DATA data { &whitePixel, sizeof(whitePixel), 0 };
    ComPtr<ID3D11Texture2D> tex;
    auto const device = G::Services::Graphics.Device;
    device->CreateTexture2D(&desc, &data, &tex);
    device->CreateShaderResourceView(tex.Get(), nullptr, &m_dummyTextureDiffuse);

    uint32 const flatNormal = 0xFFFF8080;
    data.pSysMem = &flatNormal;
    device->CreateTexture2D(&desc, &data, &tex);
    device->CreateShaderResourceView(tex.Get(), nullptr, &m_dummyTextureNormal);
}

Scene::~Scene() = default;

void Scene::Update()
{
    for (auto const& object : m_objects)
        object->Update();
}

void Scene::Render()
{
    for (auto first = true; auto const& object : m_objects)
    {
        if (dynamic_cast<Camera*>(object.get()) || dynamic_cast<Grid*>(object.get()))
            m_debugShapesMaterial->Bind();
        else if (std::exchange(first, false))
            m_basicMaterial->Bind();
        object->Render();
    }
}

void Scene::Debug()
{
    static bool rotate = true;
    float yaw = std::atan2(m_lightDirection.y, m_lightDirection.x);
    if (scoped::CollapsingHeader("Light"))
    {
        I::SetNextItemWidth(50);
        I::DragFloat("Yaw", &yaw, 0.01f);
        I::SameLine();
        I::Checkbox("Auto Rotate", &rotate);
        I::SetNextItemWidth(50);
        I::DragFloat("Height", &m_lightDirection.z, 0.01f);
    }
    if (rotate)
        yaw = I::GetTime();
    m_lightDirection.x = std::cos(yaw);
    m_lightDirection.y = std::sin(yaw);

    if (scoped::CollapsingHeader("Material"))
        m_basicMaterial->Debug();

    for (auto first = true; auto const& object : m_objects)
    {
        if (dynamic_cast<Camera*>(object.get()))
        {
            if (scoped::CollapsingHeader(std::format("Camera {}", object->GetName()).c_str()))
                object->Debug();
        }
        else if (dynamic_cast<Grid*>(object.get()))
        {
            if (scoped::CollapsingHeader(std::format("Grid {}", object->GetName()).c_str()))
                object->Debug();
        }
        else
        {
            if (std::exchange(first, false) && !I::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
                break;

            object->Debug();
        }
    }
}

Camera& Scene::CreateCameraOrbit(std::string_view name) { return Create<OrbitCamera>(name); }
Grid& Scene::CreateGrid(std::string_view name) { return Create<Grid>(name); }
Mesh& Scene::CreateMesh(std::string_view name) { return Create<Mesh>(name); }
Scene& Scene::CreateScene(std::string_view name) { return Create<Scene>(name); }

BoundingBox Scene::GetBoundingBox() const
{
    std::optional<BoundingBox> box;
    for (auto const& object : m_objects)
    {
        if (auto const mesh = dynamic_cast<Mesh*>(object.get()))
        {
            if (box)
                BoundingBox::CreateMerged(*box, *box, mesh->GetBoundingBox());
            else
                box = mesh->GetBoundingBox();
        }
    }

    return box.value_or({ Vector3::Zero, { 20, 20, 0 } });
}

}
