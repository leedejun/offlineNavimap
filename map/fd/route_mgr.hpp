#pragma once

#include "map/bookmark_manager.hpp"
#include "platform/safe_callback.hpp"
#include "map/extrapolation/extrapolator.hpp"
#include "map/routing_mark.hpp"
#include "map/framework.hpp"
#include "map/transit/transit_display.hpp"
#include "map/transit/transit_reader.hpp"

#include "routing/following_info.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_session.hpp"
#include "routing/speed_camera_manager.hpp"

#include "tracking/archival_reporter.hpp"
#include "tracking/reporter.hpp"

#include "storage/storage_defines.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/thread_checker.hpp"

#include "std/target_os.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace fd{
    typedef std::function<void(const std::string&, const std::vector<std::string>&)> BuildRouteCallback;

    class MapEngine;
    class RouteMgr;
    class RouteWrapper{
//        routing::Route route;
        std::shared_ptr<routing::Route> routePtr = nullptr;
        RouteMgr& routeMgr;
    public:
        RouteWrapper(RouteMgr&, routing::Route const &);
        ~RouteWrapper();

        void show(const std::string& fillColor, const std::string& outlineColor);
        void hide();
        bool isRouteId(const std::string& routeId);
       void getRouteTime(const std::string& routeId);

//        routing::Route& getRoute(){ return route; }
        std::shared_ptr<routing::Route> getRoutePtr(){ return routePtr; }
    private:
        std::vector<dp::DrapeID> subrouteIds;
    };

    typedef std::shared_ptr<RouteWrapper> RouteWrapperPtr ;
    class RouteMgr{
        MapEngine* engine;
        std::vector<RouteWrapperPtr> caculatedRoutes;
    public:
        RouteMgr(MapEngine* engine);
        ~RouteMgr();

        void buildRoute(std::vector<m2::PointD>& points,BuildRouteCallback callback);
        void clearRoutes();
        void showRoute(const std::string& routeId,const std::string& fillColor,const std::string& outlineColor);
        void hideRoute(const std::string& routeId);
        void enterFollowRoute(const std::string& routeId);
        double getRouteTime(const std::string& routeId);
        double getRouteDistance(const std::string& routeId);
        std::string getRouteInfo(const std::string& routeId);
        void updatePreviewMode(const std::string& routeId);
        void updatePreviewModeAll();
        void exitFollowRoute();
        void removeRoute(const bool deactivateFollowing);
        void closeRouting(const bool removeRoutePoints);
        ref_ptr<df::DrapeEngine> GetDrapeEngine();
    private:
        void showRoute(routing::Route const& route, std::string color);
        RouteWrapperPtr getRoute(const std::string& routeId);
        /// Worker thread function
        void ThreadFunc();
    };
}
