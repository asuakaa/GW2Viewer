module GW2Viewer.Data.Model;
import :Camera;
import :Scene;
import :Viewport;

namespace GW2Viewer::Data::Model
{

struct ViewportConstantBuffer
{
    Matrix World;
    Matrix ViewProjection;
    Vector3 EyePosition;
    float Padding0;
    Vector3 LightDir;
    float Padding1;
};
static_assert(!(sizeof(ViewportConstantBuffer) % 16));

Viewport::Viewport(Scene* scene): m_scene(scene)
{
    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(ViewportConstantBuffer { });
}

void Viewport::Resize(ImVec2i size)
{
    m_renderTexture.Initialize(size);
    UpdateCamera();
}

void Viewport::CameraContainScene()
{
    if (!m_camera || !m_scene)
        return;

    auto const box = m_scene->GetBoundingBox();
    auto const camera = (OrbitCamera*)m_camera;
    camera->SetTarget(box.Center);
    camera->SetRadius(Vector3(box.Extents).Length() * 2.5f);
}

void Viewport::UpdateCamera() const
{
    if (m_camera)
        m_camera->SetAspectRatio(m_renderTexture.GetAspectRatio());
}

void Viewport::Render()
{
    if (!m_scene || !m_camera)
        return;

    m_renderTexture.Bind();
    m_renderTexture.Clear({ 0.1f, 0.1f, 0.15f, 1.0f });

    ViewportConstantBuffer buffer
    {
        .World = Matrix::Identity,
        .ViewProjection = (m_camera->GetViewMatrix() * m_camera->GetProjectionMatrix()).Transpose(),
        .EyePosition = m_camera->GetPosition(),
        .LightDir = m_scene->GetLightDirection(),
    };
    buffer.LightDir.Normalize();
    m_constantBuffer.Update(buffer);
    auto context = G::Services::Graphics.Context;
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.Ptr.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.Ptr.GetAddressOf());

    m_scene->Render();
}

void Viewport::Debug()
{
}

}
