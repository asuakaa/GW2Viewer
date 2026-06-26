export module GW2Viewer.Data.Model:Mesh;
import :Common;

export namespace GW2Viewer::Data::Model
{

struct Mesh : SceneObject
{
    explicit Mesh(Scene* scene, std::string_view name) : SceneObject(scene, name) { }

    void Render() override;
    void Debug() override;

    void CreateBox();
    void LoadMesh(uint32 fvf, uint32 vertexCount, byte const* vertices, uint32 indexCount, byte const* indices, uint32 diffuseFileID, uint32 normalFileID);

    BoundingBox const& GetBoundingBox() const { return m_boundingBox; }

    bool GetVisible() const { return m_visible; }
    void SetVisible(bool value) { m_visible = value; }

private:
    BoundingBox m_boundingBox;
    uint32 m_fileDiffuse = 0;
    uint32 m_fileNormal = 0;
    bool m_visible = true;
    bool m_debugHighlightHovered = false;

    Graphics::Buffer m_vertexBuffer;
    Graphics::Buffer m_indexBuffer;
    Graphics::Buffer m_constantBuffer;
};

}
