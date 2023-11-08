#include "platform/http_client.hpp"
#include "platform/remote_file.hpp"
#include "drape_frontend/tile_key.hpp"

#include <array>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace df
{
    class DownloadRasterTiles
    {
    public:
        enum class DownloadResult
        {
            Success,
            HasDownloaded,
            IsDownloading,
            NetworkError,
            ServerError,
            AuthError,
            DiskError,
            NeedPayment,
        };
        using DownloadStartCallback = std::function<void()>;
        using DownloadFinishCallback = std::function<void(
                DownloadResult result,
                std::string const & description,
                std::string const & filePath)>;
        DownloadRasterTiles();
        virtual ~DownloadRasterTiles() = default;
        static DownloadRasterTiles & Instance();
        bool IsDownloading(TileKey const & id) const;
        bool HasDownloaded(TileKey const & id) const;
        void RegisterByTileId(TileKey const & id);
        void UnregisterByTileId(TileKey const & id);
        void RemoveDownloadingByTileId(TileKey const & id);

        // void Download(TileKey const & id, std::string const& tmpPath, DownloadStartCallback && startHandler,
        //                        DownloadFinishCallback && finishHandler);

        void Download(TileKey const & id, std::string const& filePath, DownloadFinishCallback && finishHandler);
    private:
        std::string BuildRasterTilesDownloadUrl(TileKey const & id);
        std::set<TileKey> m_downloadingIds;
        std::set<TileKey> m_registeredIds;
    };
}  // namespace df