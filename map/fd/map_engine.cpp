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

    void MapEngine::buildRoute(std::vector<m2::PointD>& points)
    {
        routeMgr.buildRoute(points);
    }
}
