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
    void MapEngine::exitFollowRoute()
    {
        routeMgr.exitFollowRoute();
    }

    void MapEngine::clearRoutes()
    {
        routeMgr.clearRoutes();
    }


}
