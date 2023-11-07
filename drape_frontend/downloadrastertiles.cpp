#include "drape_frontend/downloadrastertiles.hpp"
namespace df
{

    std::string const kRasterTilesDownloadServer = "https://webst01.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}";

    DownloadRasterTiles::DownloadRasterTiles()
    {
    }
    DownloadRasterTiles & DownloadRasterTiles::Instance()
    {
        static  DownloadRasterTiles downloadRasterTiles;
        return downloadRasterTiles;
    }

    bool DownloadRasterTiles::IsDownloading(TileKey const & id) const
    {
        return m_downloadingIds.find(id) != m_downloadingIds.cend();
    }

    bool DownloadRasterTiles::HasDownloaded(TileKey const & id) const
    {
        return m_registeredIds.find(id) != m_registeredIds.cend();
    }

    void DownloadRasterTiles::RegisterByTileId(TileKey const & id)
    {
        m_registeredIds.insert(id);
    }

    void DownloadRasterTiles::UnregisterByTileId(TileKey const & id)
    {
        m_registeredIds.erase(id);
    }

    std::string BuildRasterTilesDownloadUrl(TileKey const & id)
    {
        if (kRasterTilesDownloadServer.empty())
            return {};
        return kRasterTilesDownloadServer;
    }

    void DownloadRasterTiles::Download(TileKey const & id,std::string const& tmpPath, DownloadStartCallback && startHandler,
                               DownloadFinishCallback && finishHandler)
    {
        if (IsDownloading(id) || HasDownloaded(id))
        return;
        
        m_downloadingIds.insert(id);
        auto const path = std::move(tmpPath);

        platform::RemoteFile remoteFile(BuildRasterTilesDownloadUrl(id));
        platform::RemoteFile::Result result = remoteFile.Download(path);
        if (result.m_status ==platform::RemoteFile::Status::Ok)
        {
            m_downloadingIds.erase(id);
            RegisterByTileId(id);
        }
        
        // remoteFile.DownloadAsync(path, [startHandler = std::move(startHandler)](std::string const &)
        // {
        //     if (startHandler)
        //     startHandler();
        // }, [this, id, finishHandler = std::move(finishHandler)] (platform::RemoteFile::Result && result,
        //                                                         std::string const & filePath) mutable
        // {
        //     GetPlatform().RunTask(Platform::Thread::Network, [this, id, result = std::move(result), filePath,
        //                                                 finishHandler = std::move(finishHandler)]() mutable
        //     {
        //     m_downloadingIds.erase(id);

        //     DownloadResult downloadResult;
        //     switch (result.m_status)
        //     {
        //     case platform::RemoteFile::Status::Ok:
        //         downloadResult = DownloadResult::Success;
        //         break;
        //     case platform::RemoteFile::Status::Forbidden:
        //         downloadResult = DownloadResult::AuthError;
        //         break;
        //     case platform::RemoteFile::Status::NotFound:
        //         downloadResult = DownloadResult::ServerError;
        //         break;
        //     case platform::RemoteFile::Status::NetworkError:
        //         if (result.m_httpCode == 402)
        //         downloadResult = DownloadResult::NeedPayment;
        //         else
        //         downloadResult = DownloadResult::NetworkError;
        //         break;
        //     case platform::RemoteFile::Status::DiskError:
        //         downloadResult = DownloadResult::DiskError;
        //         break;
        //     }

        //     if (finishHandler)
        //         finishHandler(downloadResult, result.m_description, filePath);
        //     });
        // });

    }

}