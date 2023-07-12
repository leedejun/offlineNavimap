#include "route_mgr.hpp"
#include "map_engine.hpp"
#include "search/result.hpp"
#include "routing_common/ft_car_model_factory.hpp"
//#include "services/json.hpp"
#define json JsonHelper::getIns()


namespace fd{
    RouteMgr::RouteMgr(MapEngine* engine_):
        engine(engine_)
    {

    }
    RouteMgr::~RouteMgr() {

    }

    ref_ptr<df::DrapeEngine> RouteMgr::GetDrapeEngine(){
        return engine->getFramework()->GetDrapeEngine();
    }

    void RouteMgr::buildRoute(std::vector<m2::PointD>& points,BuildRouteCallback callback)
    {
        auto &routingSession = engine->getFramework()->GetRoutingManager().RoutingSession(routing::FtStrategy::MotorwayFirst);
        routing::ReadyCallbackKsp onReady;
        onReady = [&, callback, this](const std::vector<std::shared_ptr<routing::Route>> &routes,
                                      routing::RouterResultCode code) {
//            int i = 0;
            if (code == routing::RouterResultCode::NoError) {
                std::vector<std::string> routeIds;
                for (int i = 0; i < routes.size(); i++) {
                    routing::Route *route = routes[i].get();
                    routeIds.push_back(route->GetRouterId());

                    auto wrapper = std::make_shared<RouteWrapper>(*this, *route);
                    caculatedRoutes.push_back(wrapper);
                }
                callback("ok", routeIds);
            }
        };
        auto onRmRode = [&](routing::RouterResultCode code) {

        };
        auto onNeedMap = [&](uint64_t, std::set<std::string> const &) {
            callback("needMap",std::vector<std::string>());
        };

        routingSession.BuildRoute2(routing::Checkpoints(move(points)), onReady, onNeedMap,
                                   onRmRode,3);
    }

    RouteWrapperPtr RouteMgr::getRoute(const std::string& routeId)
    {
        for(int i = 0; i < caculatedRoutes.size(); i++){
            if( caculatedRoutes[i].get()->isRouteId(routeId)  )
                return caculatedRoutes[i];
        }
        return nullptr;
    }

    void RouteMgr::showRoute(const std::string& routeId,const std::string& fillColor,const std::string& outlineColor)
    {
        auto routePtr = getRoute(routeId);
        if( routePtr ){
            routePtr.get()->show(fillColor,outlineColor);
        }
    }

    void RouteMgr::hideRoute(const std::string& routeId)
    {
        auto routePtr = getRoute(routeId);
        if( routePtr ){
            routePtr.get()->hide();
        }
    }

    void RouteMgr::enterFollowRoute(const std::string& routeId)
    {
        auto routePtr = getRoute(routeId);
        if( routePtr ){
            auto &routingSession = engine->getFramework()->GetRoutingManager().RoutingSession();
            routingSession.AssignRoute(routePtr->getRoutePtr(),routing::RouterResultCode::NoError);
//            routingSession.AssignRoute(std::shared_ptr<routing::Route>(&routePtr.get()->getRoute()),routing::RouterResultCode::NoError);
            engine->getFramework()->GetRoutingManager().FollowRoute();
        }
    }
    void RouteMgr::exitFollowRoute()
    {
        engine->getFramework()->GetRoutingManager().DisableFollowMode();
    }
    void RouteMgr::removeRoute(bool deactivateFollowing)
    {
        engine->getFramework()->GetRoutingManager().RemoveRoute(deactivateFollowing);
    }
    void RouteMgr::closeRouting(bool removeRoutePoints)
    {
        engine->getFramework()->GetRoutingManager().CloseRouting(removeRoutePoints);
        engine->getFramework()->GetSearchAPI().CancelSearch(search::Mode::Everywhere);
    }

    void RouteMgr::clearRoutes()
    {
        for(int i = 0; i < caculatedRoutes.size(); i++){
            caculatedRoutes[i].get()->hide();
        }
        caculatedRoutes.clear();
    }

    //*********************************************
    drape_ptr<df::Subroute> CreateDrapeSubroute(std::vector<routing::RouteSegment> const & segments, m2::PointD const & startPt,
                                                double baseDistance, double baseDepth, bool isTransit)
    {
        auto subroute = make_unique_dp<df::Subroute>();
        subroute->m_baseDistance = baseDistance;
        subroute->m_baseDepthIndex = baseDepth;

        auto constexpr kBias = 1.0;

        if (isTransit)
        {
            subroute->m_headFakeDistance = -kBias;
            subroute->m_tailFakeDistance = kBias;
            subroute->m_polyline.Add(startPt);
            return subroute;
        }

        std::vector<m2::PointD> points;
        points.reserve(segments.size() + 1);
        points.push_back(startPt);
        for (auto const & s : segments)
            points.push_back(s.GetJunction().GetPoint());

        if (points.size() < 2)
        {
            LOG(LWARNING, ("Invalid subroute. Points number =", points.size()));
            return nullptr;
        }

        // We support visualization of fake edges only in the head and in the tail of subroute.
        auto constexpr kInvalidId = std::numeric_limits<size_t>::max();
        auto firstReal = kInvalidId;
        auto lastReal = kInvalidId;
        for (size_t i = 0; i < segments.size(); ++i)
        {
            if (!segments[i].GetSegment().IsRealSegment())
                continue;

            if (firstReal == kInvalidId)
                firstReal = i;
            lastReal = i;
        }

        if (firstReal == kInvalidId)
        {
            // All segments are fake.
            subroute->m_headFakeDistance = 0.0;
            subroute->m_tailFakeDistance = 0.0;
        }
        else
        {
            CHECK_NOT_EQUAL(firstReal, kInvalidId, ());
            CHECK_NOT_EQUAL(lastReal, kInvalidId, ());

            auto constexpr kEps = 1e-5;

            // To prevent visual artefacts, in the case when all head segments are real
            // m_headFakeDistance must be less than 0.0.
            auto const headLen = (firstReal > 0) ? segments[firstReal - 1].GetDistFromBeginningMerc() - baseDistance : 0.0;
            if (base::AlmostEqualAbs(headLen, 0.0, kEps))
                subroute->m_headFakeDistance = -kBias;
            else
                subroute->m_headFakeDistance = headLen;

            // To prevent visual artefacts, in the case when all tail segments are real
            // m_tailFakeDistance must be greater than the length of the subroute.
            auto const subrouteLen = segments.back().GetDistFromBeginningMerc() - baseDistance;
            auto const tailLen = segments[lastReal].GetDistFromBeginningMerc() - baseDistance;
            if (base::AlmostEqualAbs(tailLen, subrouteLen, kEps))
                subroute->m_tailFakeDistance = subrouteLen + kBias;
            else
                subroute->m_tailFakeDistance = tailLen;
        }

        subroute->m_polyline = m2::PolylineD(points);
        return subroute;
    }
    void FillTurnsDistancesForRendering(std::vector<routing::RouteSegment> const & segments,
                                        double baseDistance, std::vector<double> & turns)
    {
        using namespace routing::turns;
        turns.clear();
        turns.reserve(segments.size());
        for (auto const & s : segments)
        {
            auto const & t = s.GetTurn();
            CHECK_NOT_EQUAL(t.m_turn, CarDirection::Count, ());
            // We do not render some of turn directions.
            if (t.m_turn == CarDirection::None || t.m_turn == CarDirection::StartAtEndOfStreet ||
                t.m_turn == CarDirection::StayOnRoundAbout || t.m_turn == CarDirection::ReachedYourDestination)
            {
                continue;
            }
            turns.push_back(s.GetDistFromBeginningMerc() - baseDistance);
        }
    }

    RouteWrapper::RouteWrapper(RouteMgr& mgr,routing::Route const & r):
        routeMgr(mgr)
//        route(r)
    {
//        routePtr = std::shared_ptr<routing::Route>(&route);
        routePtr = std::make_shared<routing::Route>(r);
    }
    RouteWrapper::~RouteWrapper()
    {

    }

    void RouteWrapper::show(const std::string& fillColor, const std::string& outlineColor)
    {
        std::vector<routing::RouteSegment> segments;
        double distance = 0.0;
        auto const subroutesCount = routePtr->GetSubrouteCount();

        df::DrapeEngineSafePtr drapeEngine;
        drapeEngine.Set(routeMgr.GetDrapeEngine());

        for (size_t subrouteIndex = routePtr->GetCurrentSubrouteIdx(); subrouteIndex < subroutesCount; ++subrouteIndex) {
            routePtr->GetSubrouteInfo(subrouteIndex, segments);

            auto const startPt = routePtr->GetSubrouteAttrs(subrouteIndex).GetStart().GetPoint();
            auto subroute = CreateDrapeSubroute(segments, startPt, distance,
                                                static_cast<double>(subroutesCount - subrouteIndex -
                                                                    1), false);
            if (!subroute)
                continue;
            distance = segments.back().GetDistFromBeginningMerc();
            subroute->m_routeType = df::RouteType::Car;
            subroute->AddStyle(df::SubrouteStyle(fillColor, outlineColor));
            FillTurnsDistancesForRendering(segments, subroute->m_baseDistance, subroute->m_turns);
            auto const subrouteId = drapeEngine.SafeCallWithResult(&df::DrapeEngine::AddSubroute,
                                                                   df::SubrouteConstPtr(
                                                                           subroute.release()));
            subrouteIds.push_back(subrouteId);

        }
    }

    void RouteWrapper::hide()
    {
        df::DrapeEngineSafePtr drapeEngine;
        drapeEngine.Set(routeMgr.GetDrapeEngine());
        for(auto subrouteId : subrouteIds){
            drapeEngine.SafeCall(&df::DrapeEngine::RemoveSubroute,subrouteId,false);
        }
    }

    bool RouteWrapper::isRouteId(const std::string& routeId)
    {
        return routePtr->GetRouterId() == routeId;
    }


    double RouteMgr::getRouteTime(const std::string& routeId)
    {
        RouteWrapperPtr RouteMgr = getRoute(routeId);
//        routing::Route  route = RouteMgr.get()->getRoute();
//        route.GetTotalTimeSec();
//        return  route.GetTotalTimeSec();

        return RouteMgr->getRoutePtr()->GetTotalTimeSec();
    }
    double RouteMgr::getRouteDistance(const std::string& routeId)
    {
        RouteWrapperPtr RouteMgr = getRoute(routeId);
//        routing::Route  route = RouteMgr.get()->getRoute();
//        return  route.GetTotalDistanceMeters();

        return RouteMgr->getRoutePtr()->GetTotalDistanceMeters();

    }

    void  RouteMgr::updatePreviewModeAll(){

        m2::Rect<double> sumrect;
        for(int i = 0; i < caculatedRoutes.size(); i++){
            RouteWrapperPtr RouteMgr  =caculatedRoutes[i];
            m2::PolylineD polyline = RouteMgr->getRoutePtr()->GetPoly();
            m2::Rect<double> rect = polyline.GetLimitRect();
            sumrect.Add(rect);
        }
        sumrect.Scale(2.5);
        df::DrapeEngineSafePtr drapeEngine;
        drapeEngine.Set(GetDrapeEngine());
        drapeEngine.SafeCall(&df::DrapeEngine::DeactivateRouteFollowing);
        drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, sumrect,
                              true /* applyRotation */, -1 /* zoom */, true /* isAnim */,
                              true /* useVisibleViewport */);
    }
    void  RouteMgr::updatePreviewMode(const std::string& routeId){

        for(int i = 0; i < caculatedRoutes.size(); i++){
            if( caculatedRoutes[i].get()->isRouteId(routeId)  ){
                RouteWrapperPtr RouteMgr  =caculatedRoutes[i];
                m2::PolylineD polyline = RouteMgr->getRoutePtr()->GetPoly();
                m2::Rect<double> rect = polyline.GetLimitRect();
                rect.Scale(2.5);
                df::DrapeEngineSafePtr drapeEngine;
                drapeEngine.Set(GetDrapeEngine());
                drapeEngine.SafeCall(&df::DrapeEngine::DeactivateRouteFollowing);
                drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, rect,
                                     true /* applyRotation */, -1 /* zoom */, true /* isAnim */,
                                     true /* useVisibleViewport */);
            }
        }

    }
    std::string RouteMgr::getRouteInfo(const std::string& routeId)
    {
        RouteWrapperPtr RouteMgr = getRoute(routeId);
//        routing::Route  route = RouteMgr.get()->getRoute();
//        std::vector<routing::RouteSegment> segment = route.GetRouteSegments();
        std::vector<routing::RouteSegment> segment =  RouteMgr->getRoutePtr()->GetRouteSegments();
        std::vector<routing::RouteSegment>::const_iterator sItor = segment.begin(), sLast = segment.end();
        std::stringstream stream;
        stream.precision(9);
        int i = 0;

        double first_distance = 0.0;
        std::string first_street = "";
        std::vector<std::pair<double,double>> first_lonlat;

        double second_distance = 0.0;
        std::string second_street = "";
        std::vector<std::pair<double,double>> second_lonlat;

        double pre_distance = 0.0;
        std::string pre_street = "";

        auto fn_to_stream = [&stream, &i]
                (std::string const& street, double distance, std::vector<std::pair<double,double>>const & vec_lonlat){
            stream << "{";
            stream << "\"index\":" << "\"" << i++ << "\",";
            stream << "\"m_street\":" << "\"" << street << "\",";
            stream << "\"distance\":" << "\"" << distance << "\",";
            stream << "\"lonlat\":" << "\"";
            std::stringstream stream_lonlat;
            stream_lonlat.precision(9);
            for(auto &item:vec_lonlat){
                stream_lonlat << item.first<<","<<item.second<<";";
            }
            std::string str_lonlat = stream_lonlat.str();
            if(str_lonlat!="") stream << str_lonlat.substr(0, str_lonlat.size()-1);
            stream << "\",";
            stream << "\"turnString\":" << "\"" << "直行" << "\",";
            stream << "\"targetName\":" << "\"" << street << "\",";
            stream << "\"sourceName\":" << "\"" << street << "\"";
            stream << "}";
            stream << ",";
        };

        for (; sItor != sLast; ++sItor) {
            std::string street = sItor->GetStreet();
            street=street==""?"无名路":street;
            double cur_distance = sItor->GetDistFromBeginningMeters();
            double mercatorLon = sItor->GetJunction().GetPoint().x;
            double mercatorLat =  sItor->GetJunction().GetPoint().y;
            m2::PointD tam = m2::PointD(mercatorLon, mercatorLat);
//        墨卡托转wgs84
            ms::LatLon latLon = mercator::ToLatLon(tam);
            double doubleLon =latLon.m_lon;
            double doubleLat =latLon.m_lat;
            if(sItor->GetTurn().m_turn!= routing::turns::CarDirection::GoStraight&&sItor->GetTurn().m_turn!=routing::turns::CarDirection::None)
            {
                //拐弯了。
                if(first_street!=""){
                    //写入first
                    fn_to_stream(first_street, first_distance, first_lonlat);
                    first_street = "";
                    first_lonlat.clear();
                }
                if(second_street!=""){
                    //写入second
                    fn_to_stream(second_street, second_distance, second_lonlat);
                    second_street = "";
                    second_lonlat.clear();
                }
                stream << "{";
                stream << "\"index\":" << "\"" << i++ << "\",";
                stream << "\"m_street\":" << "\"" << street << "\",";
                stream << "\"distance\":" << "\"" << cur_distance - pre_distance << "\",";
                stream << "\"lonlat\":" << "\"" << doubleLon <<","<< doubleLat << "\",";
                stream << "\"turnString\":" << "\"" << GetTurnString(sItor->GetTurn().m_turn) << "\",";
                stream << "\"targetName\":" << "\"" << street << "\",";
                stream << "\"sourceName\":" << "\"" << pre_street << "\"";
                stream << "}";
                stream << ",";
            }else if(street==second_street){
                //直行，同路，无名路
                second_distance += cur_distance - pre_distance;
                second_lonlat.push_back({doubleLon, doubleLat});
            }else if(street==first_street) {
                //直行，同路
                if (second_street == "") {
                    //同名路，中间没有有无名路，合并到 first_street。
                    first_distance += cur_distance - pre_distance;
                    first_lonlat.push_back({doubleLon, doubleLat});
                } else {
                    //同名路，中间有无名路，second_street 合并到 first_street。
                    first_distance += second_distance;
                    first_distance += cur_distance - pre_distance;
                    first_lonlat.insert(first_lonlat.end(), second_lonlat.begin(), second_lonlat.end());
                    first_lonlat.push_back({doubleLon, doubleLat});
                    second_street = "";
                    second_distance = 0.0;
                    second_lonlat.clear();
                }
            } else if(first_street==""){
                //直行，新路，前面没有路
                first_street = street;
                first_distance = cur_distance - pre_distance;
                first_lonlat.push_back({doubleLon, doubleLat});
                second_street = "";
                second_distance = 0.0;
                second_lonlat.clear();
            }else if(street=="无名路"){
                //直行，遇到无名路
                second_street = street;
                second_distance = cur_distance - pre_distance;
                second_lonlat.push_back({doubleLon, doubleLat});
            } else {
                //直行，新路，前面有路
                //写入first
                fn_to_stream(first_street, first_distance, first_lonlat);
                if(second_street != ""){
                    fn_to_stream(second_street, second_distance, second_lonlat);
                }
                first_street = street;
                first_distance = cur_distance - pre_distance;
                first_lonlat.push_back({doubleLon, doubleLat});
                second_street = "";
                second_distance = 0.0;
                second_lonlat.clear();
            }

            pre_distance = cur_distance;
            pre_street = street;
        }

        if(first_street!=""){
            //写入first
            fn_to_stream(first_street, first_distance, first_lonlat);
        }

        if(second_street!=""){
            //写入second
            fn_to_stream(second_street, second_distance, second_lonlat);
        }

        auto str_stream = stream.str();
        return  str_stream==""?"[]":"["+str_stream.substr(0,str_stream.size()-1)+"]";
    }
};
