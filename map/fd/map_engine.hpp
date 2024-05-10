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

#include "route_mgr.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace fd{
    class MapEngine{
        MapEngine();
    public:
        ~MapEngine();

        static MapEngine& get(Framework* frmWork = nullptr);

        // route functions
        // 算路
        // 更改路线样式
        void buildRoute(std::vector<m2::PointD>& points,BuildRouteCallback callback);
        void clearRoutes();
        void showRoute(const std::string& routeId,const std::string& fillColor,const std::string& outlineColor);
        void hideRoute(const std::string& routeId);
        void enterFollowRoute(const std::string& routeId);
        double getRouteTime(const std::string& routeId);
        double getRouteDistance(const std::string& routeId);
        std::string getRouteInfo(const std::string& routeId);
        void exitFollowRoute();
        void updatePreviewModeAll();
        void updatePreviewMode(const std::string& routeId);
        void removeRoute(const bool deactivateFollowing);
        void closeRouting(const bool removeRoutePoints);

        //
        void getAvaliableRoute();

        Framework* getFramework(){ return framework; }
    private:
        Framework* framework;
        RouteMgr routeMgr;
    };
}
