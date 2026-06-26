export module GW2Viewer.Data.Texture.Manager;
import GW2Viewer.Common;
import GW2Viewer.Data.Texture;
import std;
import <concurrentqueue/blockingconcurrentqueue.h>;

export namespace GW2Viewer::Data::Texture
{

class Manager
{
public:
    TextureEntry const* Get(uint32 fileID, LoadTextureOptions const& options = { });
    TextureEntry const* GetEntry(uint32 fileID);
    std::unique_ptr<Texture> Create(uint32 width, uint32 height, void const* data = nullptr);
    TextureEntry const* Load(uint32 fileID, LoadTextureOptions const& options = { });
    uint32 Load(std::filesystem::path const& localFilePath, LoadTextureOptions const& options = { });
    void UploadToGPU();
    void StopLoading()
    {
        m_loadingThreadExitRequested = true;
        if (m_loadingThread)
            m_loadingThread->join();
    }

private:
    std::unordered_map<uint32, std::shared_ptr<TextureEntry>> m_textures;
    std::recursive_mutex m_mutex;
    uint32 m_nextLocalFileID = -1;

    struct BoxedImage { void* ScratchImage; ~BoxedImage(); };
    std::unique_ptr<BoxedImage> GetTextureRGBAImage(TextureEntry& texture);

    moodycamel::BlockingConcurrentQueue<std::weak_ptr<TextureEntry>> m_loadingQueue;
    moodycamel::ConcurrentQueue<std::pair<std::weak_ptr<TextureEntry>, std::unique_ptr<BoxedImage>>> m_uploadQueue;

    std::optional<std::thread> m_loadingThread;
    bool m_loadingThreadExitRequested = false;

    void LoadingThread();
};

}
