#include "drape_frontend/downloadrastertiles.hpp"
#include "geometry/mercator.hpp"
#include "base/string_format.hpp"
#include <iostream>
#include "3party/httplib/httplib.h"
#include "coding/file_writer.hpp"
#include <pthread.h>



namespace df
{

//    std::string const kRasterTilesDownloadServer = "https://webst01.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}";
////    std::string const kRasterTilesDownloadServer = "https://webst01.is.autonavi.com/appmaptile?style=6";
//    std::string const kRasterTilesDownloadServer = "https://webst01.is.autonavi.com";
//    std::string const kRasterTilesDownloadStylesServer = "/appmaptile?style=6";


//    std::string const kRasterTilesDownloadHost = "http://192.168.3.252";
    std::string const kRasterTilesDownloadHost = "192.168.3.252";
    std::string const kRasterTilesDownloadStyles = "/tiles/satellite";
    int const kRasterTilesDownloadPort = 8000;

//    http://192.168.3.252:8000/styles/basic/{z}/{x}/{y}.png

//    http://mt1.google.cn/vt/lyrs=s&x={x}&y={y}&z={z}

//    std::string const kRasterTilesDownloadServer = "http://mt1.google.cn";
//    std::string const kRasterTilesDownloadStylesServer ="/vt/lyrs=s";




    DownloadRasterTiles::DownloadRasterTiles()
    {
    }
    DownloadRasterTiles & DownloadRasterTiles::Instance()
    {
        static  DownloadRasterTiles downloadRasterTiles;
        return downloadRasterTiles;
    }

    bool DownloadRasterTiles::IsDownloading(TileKey const & id)
    {
        std::lock_guard<std::mutex> lock(m_downloadingMutex);
        bool ret = (m_downloadingIds.find(id) != m_downloadingIds.cend());
        return ret;
    }

    bool DownloadRasterTiles::HasDownloaded(TileKey const & id)
    {
        std::lock_guard<std::mutex> lock(m_registeredMutex);
        bool ret = (m_registeredIds.find(id) != m_registeredIds.cend());
        return ret;
    }

    void DownloadRasterTiles::RegisterByTileId(TileKey const & id)
    {
        std::lock_guard<std::mutex> lock(m_registeredMutex);
        m_registeredIds.insert(id);
    }

    void DownloadRasterTiles::UnregisterByTileId(TileKey const & id)
    {
        std::lock_guard<std::mutex> lock(m_registeredMutex);
        m_registeredIds.erase(id);
    }

    void DownloadRasterTiles::RemoveDownloadingByTileId(TileKey const & id)
    {
        std::lock_guard<std::mutex> lock(m_downloadingMutex);
        m_downloadingIds.erase(id);
    }

    std::string DownloadRasterTiles::BuildRasterTilesDownloadUrl(TileKey const & id)
    {
        if (kRasterTilesDownloadHost.empty())
            return {};


        auto tileRect = id.GetGlobalRect();
        if (id.m_zoomLevel==1)
        {
            tileRect = id.GetGlobalRectSmall();
        }

        m2::PointD tileCenter = tileRect.Center();
        double lon = mercator::XToLon(tileCenter.x);
        double lat = mercator::YToLat(tileCenter.y);

        double n = std::pow(2, id.m_zoomLevel-1);
        double xtile = n * (double(lon + 180.0) / double(360.0));
        double ytile = n * (1.0 - (log(tan(lat * math::pi / double(180.0)) + double(1.0) / cos(lat * M_PI / double(180.0))) / math::pi)) / double(2.0);
        int webX = (int) std::floor(xtile);
        int webY = (int) std::floor(ytile);
        std::string strZ = strings::ToString(id.m_zoomLevel-1);

        if (id.m_zoomLevel==1)
        {
            n = std::pow(2, id.m_zoomLevel);
            xtile = n * (double(lon + 180.0) / double(360.0));
            ytile = n * (1.0 - (log(tan(lat * math::pi / double(180.0)) + double(1.0) / cos(lat * M_PI / double(180.0))) / math::pi)) / double(2.0);
            webX = (int) std::floor(xtile);
            webY = (int) std::floor(ytile);
            strZ = strings::ToString(id.m_zoomLevel);
        }

        std::stringstream ss;
        ss<<kRasterTilesDownloadStyles<<"/"<<strZ<<"/"<<webX<<"/"<<webY<<".png";
//        ss<<kRasterTilesDownloadStylesServer<<"&x="<<webX<<""
//                                      <<"&y="<<webY<<""
//                                      <<"&z="<<strZ<<"";


        return ss.str();
    }

    void DownloadRasterTiles::Download(TileKey const & id,std::string const& tmpPath
                               ,DownloadFinishCallback && finishHandler)
    {

        try {

            DownloadRasterTiles::DownloadResult result;
            std::string description = "";
            std::string filePath = tmpPath;
            if (IsDownloading(id)) {
                description = "IsDownloading";
                result = DownloadRasterTiles::DownloadResult::IsDownloading;
                return;
            }

            if (HasDownloaded(id)) {
                description = "HasDownloaded";
                result = DownloadRasterTiles::DownloadResult::HasDownloaded;
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_downloadingMutex);
                m_downloadingIds.insert(id);
            }

            auto const path = std::move(tmpPath);
            httplib::Client client(kRasterTilesDownloadHost.c_str(),
                                   kRasterTilesDownloadPort); //http
            if (auto res = client.Get(BuildRasterTilesDownloadUrl(id).c_str())) {
                if (res->status == 200) {
                    description = res->body;
                    result = DownloadRasterTiles::DownloadResult::Success;
                } else {
                    description = res->reason;
                    result = DownloadRasterTiles::DownloadResult::ServerError;
                }

            } else {
                std::cout << res.error() << std::endl;
                description = res.error();
                result = DownloadRasterTiles::DownloadResult::NetworkError;
            }

            if (finishHandler) {
                finishHandler(id, result, description, filePath);
            }
        }
        catch (std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
            LOG(LWARNING, ("Error: ", e.what()));
        }
    }

}