//
// Created by wzd on 2022/4/13.
//

#ifndef ANDROID_FT_ROUTING_MANAGER_HPP
#define ANDROID_FT_ROUTING_MANAGER_HPP
#include "../map/routing_manager.hpp"
namespace routing {

    class FtRoutingManager : public RoutingManager{
    public:

        void CalcRoute();
        void SelectRoute();
        void StyleRoute();
        void DeleteRoute();
        void SaveRoute();
        void FollowRoute();
        void GetRouteInfo();

    };

}
#endif //ANDROID_FT_ROUTING_MANAGER_HPP
