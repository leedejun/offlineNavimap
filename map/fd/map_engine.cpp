#include "map_engine.hpp"

namespace fd{
    MapEngine::MapEngine():
        routeMgr(this)
    {
    }
    MapEngine::~MapEngine() {}

    MapEngine& MapEngine::get(Framework* frm){
        static MapEngine ins;
        if( frm ){
            ins.framework = frm;
        }
        return ins;
    }

    void MapEngine::buildRoute(std::vector<m2::PointD>& points,BuildRouteCallback callback)
    {
        routeMgr.buildRoute(points,callback);
    }

    void MapEngine::showRoute(const std::string& routeId,const std::string& fillColor,const std::string& outlineColor)
    {
        routeMgr.showRoute(routeId,fillColor,outlineColor);
    }

    void MapEngine::hideRoute(const std::string& routeId)
    {
        routeMgr.hideRoute(routeId);
    }

    void MapEngine::enterFollowRoute(const std::string& routeId)
    {
        routeMgr.enterFollowRoute(routeId);
    }
    double MapEngine::getRouteTime(const std::string& routeId)
    {
        return  routeMgr.getRouteTime(routeId);

    }
    double MapEngine::getRouteDistance(const std::string& routeId)
    {
        return  routeMgr.getRouteDistance(routeId);

    }
    std::string  MapEngine::getRouteInfo(const std::string& routeId)
    {
        return  routeMgr.getRouteInfo(routeId);

    }
    void MapEngine::exitFollowRoute()
    {
        routeMgr.exitFollowRoute();
    }
    void MapEngine::updatePreviewModeAll()
    {
        routeMgr.updatePreviewModeAll();
    }
    void MapEngine::updatePreviewMode(const std::string& routeId)
    {
        routeMgr.updatePreviewMode(routeId);
    }
    void MapEngine::removeRoute(bool deactivateFollowing)
    {
        routeMgr.removeRoute(deactivateFollowing);
    }
    void MapEngine::closeRouting(bool removeRoutePoints)
    {
        routeMgr.closeRouting(removeRoutePoints);
    }

    void MapEngine::clearRoutes()
    {
        routeMgr.clearRoutes();
    }


}
