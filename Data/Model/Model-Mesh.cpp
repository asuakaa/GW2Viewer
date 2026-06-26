module GW2Viewer.Data.Model;
import :Mesh;
import :Scene;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Texture;
#include "Macros.h"

namespace GW2Viewer::Data::Model
{

struct MeshConstantBuffer
{
    uint32 HighlightObject : 1 = false;
    float Padding0;
    float Padding1;
    float Padding2;
};
static_assert(!(sizeof(MeshConstantBuffer) % 16));

void Mesh::Render()
{
    if (!m_visible)
        return;

    ID3D11ShaderResourceView* textures[] { GetScene()->GetDummyTextureDiffuse(), GetScene()->GetDummyTextureNormal() };
    if (auto const texture = G::Game.Texture.Get(m_fileDiffuse))
        textures[0] = (ID3D11ShaderResourceView*)texture->Texture->Handle.GetTexID();
    if (auto const texture = G::Game.Texture.Get(m_fileNormal))
        textures[1] = (ID3D11ShaderResourceView*)texture->Texture->Handle.GetTexID();

    uint32 stride = sizeof(Vertex);
    uint32 offset = 0;

    auto context = G::Services::Graphics.Context;
    //context->IASetInputLayout(m_inputLayout.Get());
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.Ptr.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Ptr.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->PSSetShaderResources(0, 2, textures);

    MeshConstantBuffer buffer
    {
        .HighlightObject = m_debugHighlightHovered,
    };
    m_constantBuffer.Update(buffer);
    context->VSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_constantBuffer.Ptr.GetAddressOf());

    context->DrawIndexed(m_indexBuffer.Count, 0, 0);

    m_constantBuffer.Update(MeshConstantBuffer { });
}

void Mesh::Debug()
{
    scoped::WithID(this);
    I::Checkbox(GetName().c_str(), &m_visible);
    m_debugHighlightHovered = I::IsItemHovered();
}

void Mesh::CreateBox()
{
    std::array<Vertex, 8> const vertices
    { {
        { .Position = {-0.5f, -0.5f,  0.5f}, .Color = { 0, 0, 0, 1 } },
        { .Position = { 0.5f, -0.5f,  0.5f}, .Color = { 1, 0, 0, 1 } },
        { .Position = { 0.5f,  0.5f,  0.5f}, .Color = { 0, 1, 0, 1 } },
        { .Position = {-0.5f,  0.5f,  0.5f}, .Color = { 0, 0, 1, 1 } },
        { .Position = {-0.5f, -0.5f, -0.5f}, .Color = { 1, 1, 0, 1 } },
        { .Position = { 0.5f, -0.5f, -0.5f}, .Color = { 1, 0, 1, 1 } },
        { .Position = { 0.5f,  0.5f, -0.5f}, .Color = { 0, 1, 1, 1 } },
        { .Position = {-0.5f,  0.5f, -0.5f}, .Color = { 1, 1, 1, 1 } },
    } };
    std::array<uint16, 2 * 3 * 6> const indices
    {
        4, 7, 6,   4, 6, 5,
        3, 2, 1,   3, 1, 0,
        7, 6, 2,   7, 2, 3,
        4, 5, 1,   4, 1, 0,
        5, 6, 2,   5, 2, 1,
        7, 4, 0,   7, 0, 3,
    };
    m_vertexBuffer = G::Services::Graphics.CreateVertexBuffer(vertices);
    m_indexBuffer = G::Services::Graphics.CreateIndexBuffer(indices);
    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(MeshConstantBuffer { });
}

enum class FVFFlags : uint32
{
    None               = 0,
    Position           = 0x00000001,
    BlendWeight        = 0x00000002,
    BlendIndices       = 0x00000004,
    Normal             = 0x00000008,
    Color              = 0x00000010,
    Tangent            = 0x00000020,
    Binormal           = 0x00000040,
    TangentFrame       = 0x00000080,
    UV32Mask           = 0x0000ff00,
    UV16Mask           = 0x00ff0000,
    Unknown1           = 0x01000000,
    Unknown2           = 0x02000000,
    Unknown3           = 0x04000000,
    Unknown4           = 0x08000000,
    PositionCompressed = 0x10000000,
    Unknown5           = 0x20000000
};
struct FVFInfo
{
    uint32 Size = 0;
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout;
};
std::unordered_map<FVFFlags, FVFInfo> cachedInputLayouts;
FVFInfo const* BuildLayout(FVFFlags fvfMask)
{
    auto& cached = cachedInputLayouts[fvfMask];
    if (cached.Size)
        return &cached;

    FVFInfo info;
    auto add = [&info](char const* name, uint32 index, DXGI_FORMAT format)
    {
        info.Layout.push_back(
        {
            .SemanticName         = name,
            .SemanticIndex        = index,
            .Format               = format,
            .AlignedByteOffset    = info.Size,
            .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
        });
        info.Size += DirectX::BitsPerPixel(format) / 8;
    };

    if (fvfMask & FVFFlags::Position)       add("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    if (fvfMask & FVFFlags::BlendWeight)    add("BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (fvfMask & FVFFlags::BlendIndices)   add("BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT);
    if (fvfMask & FVFFlags::Normal)         add("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    if (fvfMask & FVFFlags::Color)          add("COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (fvfMask & FVFFlags::Tangent)        add("TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    if (fvfMask & FVFFlags::Binormal)       add("BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    if (fvfMask & FVFFlags::TangentFrame)   add("TANGENTFRAME", 0, DXGI_FORMAT_R32G32B32_FLOAT);

    uint32 texCoordIndex = 0;
    for (auto i = 8; i < 24; ++i)
        if (fvfMask & FVFFlags(1 << i))
            add("TEXCOORD", texCoordIndex++, i < 16 ? DXGI_FORMAT_R32G32_FLOAT : DXGI_FORMAT_R16G16_FLOAT);

    uint32 padIndex = 0;
    if (fvfMask & FVFFlags::Unknown1)
{
        add("PADDING", padIndex++, DXGI_FORMAT_R32G32B32A32_FLOAT);
        add("PADDING", padIndex++, DXGI_FORMAT_R32G32B32A32_FLOAT);
        add("PADDING", padIndex++, DXGI_FORMAT_R32G32B32A32_FLOAT);
    }
    if (fvfMask & FVFFlags::Unknown2) add("PADDING", padIndex++, DXGI_FORMAT_R32_FLOAT);
    if (fvfMask & FVFFlags::Unknown3) add("PADDING", padIndex++, DXGI_FORMAT_R32_FLOAT);
    if (fvfMask & FVFFlags::Unknown4) add("PADDING", padIndex++, DXGI_FORMAT_R32G32B32A32_FLOAT);
    if (fvfMask & FVFFlags::PositionCompressed) add("POSITION", 0, DXGI_FORMAT_R11G11B10_FLOAT);
    if (fvfMask & FVFFlags::Unknown5) add("PADDING", padIndex++, DXGI_FORMAT_R32G32B32_FLOAT);

    cached = std::move(info);
    return &cached;
}

void Mesh::LoadMesh(uint32 fvf, uint32 vertexCount, byte const* vertices, uint32 indexCount, byte const* indices, uint32 diffuseFileID, uint32 normalFileID)
{
    auto const info = BuildLayout((FVFFlags)fvf);
    //device->CreateInputLayout(info->Layout.data(), info->Layout.size(), shaderCode, shaderSize, &m_inputLayout);
    uint32 offsetPosition = 0, offsetNormal = 0, offsetTangent = 0, offsetBitangent = 0, offsetTangentFrame = 0, offsetUV = 0, offsetUVHalf = 0;
    for (auto const& element : info->Layout)
    {
        if (element.SemanticName == "POSITION"sv)
            offsetPosition = element.AlignedByteOffset;
        else if (element.SemanticName == "NORMAL"sv)
            offsetNormal = element.AlignedByteOffset;
        else if (element.SemanticName == "TANGENT"sv)
            offsetTangent = element.AlignedByteOffset;
        else if (element.SemanticName == "BITANGENT"sv)
            offsetBitangent = element.AlignedByteOffset;
        else if (element.SemanticName == "TANGENTFRAME"sv)
            offsetTangentFrame = element.AlignedByteOffset;
        else if (element.SemanticName == "TEXCOORD"sv && element.SemanticIndex == 0 && element.Format == DXGI_FORMAT_R32G32_FLOAT)
            offsetUV = element.AlignedByteOffset;
        else if (element.SemanticName == "TEXCOORD"sv && element.SemanticIndex == 0 && element.Format == DXGI_FORMAT_R16G16_FLOAT)
            offsetUVHalf = element.AlignedByteOffset;
    }

    std::vector<Vertex> final;
    final.reserve(vertexCount);
    for (auto i = 0; i < vertexCount; ++i)
    {
        auto& vertex = final.emplace_back();
        vertex.Position = *(Vector3*)&vertices[offsetPosition];
        if (offsetUV)
            vertex.UV = *(Vector2*)&vertices[offsetUV];
        else if (offsetUVHalf)
            vertex.UV = DirectX::PackedVector::XMLoadHalf2((DirectX::PackedVector::XMHALF2*)&vertices[offsetUVHalf]);
        if (offsetNormal)
            vertex.Normal = *(Vector3*)&vertices[offsetNormal];
        if (offsetTangent)
            vertex.Tangent = *(Vector3*)&vertices[offsetTangent];
        if (offsetBitangent)
            vertex.Bitangent = *(Vector3*)&vertices[offsetBitangent];
        if (offsetTangentFrame)
        {
            vertex.Normal = DirectX::PackedVector::XMLoadUByteN4((DirectX::PackedVector::XMUBYTEN4*)&vertices[offsetTangentFrame]);
            vertex.Tangent = DirectX::PackedVector::XMLoadUByteN4((DirectX::PackedVector::XMUBYTEN4*)&vertices[offsetTangentFrame + 4]);
            vertex.Bitangent = DirectX::PackedVector::XMLoadUByteN4((DirectX::PackedVector::XMUBYTEN4*)&vertices[offsetTangentFrame + 8]);
        }
        vertex.Color = Vector4 { 1, 1, 1, 1 };
        vertices += info->Size;
    }
    if (auto const file = G::Game.Archive.GetFileEntry(diffuseFileID))
        diffuseFileID = file->GetBestVersion().ID;
    m_fileDiffuse = diffuseFileID;
    if (auto const file = G::Game.Archive.GetFileEntry(normalFileID))
        normalFileID = file->GetBestVersion().ID;
    m_fileNormal = normalFileID;
    m_vertexBuffer = G::Services::Graphics.CreateVertexBuffer(final);
    m_indexBuffer = G::Services::Graphics.CreateIndexBuffer(std::span { (uint16*)indices, indexCount });
    m_constantBuffer = G::Services::Graphics.CreateConstantBuffer(MeshConstantBuffer { });
    BoundingBox::CreateFromPoints(m_boundingBox, final.size(), &final.data()->Position, sizeof(Vertex));
}

}
