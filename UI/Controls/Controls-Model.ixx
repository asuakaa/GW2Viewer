export module GW2Viewer.UI.Controls:Model;
import GW2Viewer.Common;
import GW2Viewer.Common.FourCC;
import GW2Viewer.Common.Token64;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Model;
import GW2Viewer.Data.Pack.PackFile;
import GW2Viewer.Services.Graphics;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Controls
{

struct Model
{
    Data::Model::Scene Scene { nullptr, "" };
    Data::Model::Viewport Viewport { &Scene };
    bool Panning = false;
    bool Rotating = false;
    std::unique_ptr<Data::Pack::PackFile> PackFile;

    Model(bool grid = false)
    {
        if (grid)
            Scene.CreateGrid("").Initialize(10, 1.0f);
        Viewport.SetCamera(&Scene.CreateCameraOrbit(""));
    }

    bool Load(uint32 fileID)
    {
        if (PackFile = G::Game.Archive.GetPackFile(fileID); PackFile && PackFile->Header.ContentType == fcc::MODL)
            return Load(*PackFile);

        return false;
    }
    bool Load(Data::Pack::PackFile const& file)
    {
        if (file.Header.ContentType != fcc::MODL || !file.HasChunk(fcc::GEOM))
            return false;

        for (auto const mesh : file.QueryChunk(fcc::GEOM)["meshes"])
        {
            auto const geometry = mesh["geometry"];
            auto const verts = geometry["verts"];
            uint32 fileDiffuse = 0;
            uint32 fileNormal = 0;
            if (int32 const materialIndex = mesh["materialIndex"]; materialIndex >= 0)
                if (auto const material = file.QueryChunk(fcc::MODL)["permutations"][0u]["materials"][materialIndex])
                    if (auto const textures = material["textures"])
                        for (auto const texture : textures)
                            if ((Token64)texture["token"] == Token64("diffuse tex"))
                                fileDiffuse = texture["filename"];
                            else if ((Token64)texture["token"] == Token64("normal tex"))
                                fileNormal = texture["filename"];
            auto const meshName = ((Token64)mesh["meshName"]).GetString();
            std::string_view materialName = mesh["materialName"];

            auto& m = Scene.CreateMesh(!materialName.empty() ? std::format("{} / {}", meshName.data(), materialName) : meshName.data());
            m.LoadMesh(verts["mesh"]["fvf"],
                verts["vertexCount"],
                verts["mesh"]["vertices[]"].GetPointer(),
                geometry["indices"]["indices[]"].GetArraySize(),
                geometry["indices"]["indices[]"].GetPointer(),
                fileDiffuse,
                fileNormal);
            m.SetVisible(!((uint32)mesh["flags"] & 4)); // LOD
        }
        Viewport.CameraContainScene();
        return true;
    }

    struct DrawOptions
    {
        ImVec2 Size = I::GetContentRegionAvail();
        bool UI = false;
    };
    void Draw(DrawOptions const& options = { })
    {
        auto const size = ImMax(options.Size, { 1, 1 });
        Scene.Update();
        Viewport.Resize({ (int32)size.x, (int32)size.y });
        Viewport.Render();
        auto const cursor = I::GetCursorScreenPos();
        if (auto const texture = G::Game.Texture.Get(G::UI.Textures.Transparency))
        {
            ImVec2 const texSize { (float)texture->Texture->Width, (float)texture->Texture->Height };
            for (ImVec2 pos; pos.x < size.x; pos.x += texture->Texture->Width)
                for (pos.y = 0; pos.y < size.y; pos.y += texture->Texture->Height)
                    I::GetWindowDrawList()->AddImage(texture->Texture->Handle, cursor + pos, ImMin(cursor + pos + texSize, cursor + size), { }, ImMin(ImVec2(size - pos) / texSize, { 1, 1 }));
        }
        I::Image((ImTextureID)Viewport.GetShaderResourceView(), size);

        if (I::IsItemHovered())
        {
            if (I::IsMouseDown(ImGuiMouseButton_Middle))
                Panning = true;
            if (I::IsMouseDown(ImGuiMouseButton_Left))
                Rotating = true;
        }
        if (!I::IsMouseDown(ImGuiMouseButton_Middle))
            Panning = false;
        if (!I::IsMouseDown(ImGuiMouseButton_Left))
            Rotating = false;

        if (I::IsItemHovered() || Panning || Rotating)
        {
            auto const pan = Panning ? I::GetIO().MouseDelta : ImVec2();
            auto const rotation = Rotating ? I::GetIO().MouseDelta : ImVec2();
            auto const zoom = I::GetIO().MouseWheel;
            if (pan.x || pan.y || rotation.x || rotation.y || zoom)
                Viewport.GetCamera()->HandleInput(pan, rotation, zoom);
        }

        if (options.UI)
        {
            auto const oldFrameRounding = I::GetStyle().FrameRounding;
            if (scoped::WithCursorScreenPos(cursor))
            if (scoped::WithStyleVar(ImGuiStyleVar_FrameRounding, 0))
            if (scoped::Child(I::GetSharedScopeID("Controls:Model"), { 200, -FLT_MIN }, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
            if (scoped::WithStyleVar(ImGuiStyleVar_FrameRounding, oldFrameRounding))
            {
                if (scoped::WithStyleVarY(ImGuiStyleVar_ItemSpacing, 0))
                {
                    Viewport.Debug();
                    Scene.Debug();
                }
            }
        }
    }
};

}
