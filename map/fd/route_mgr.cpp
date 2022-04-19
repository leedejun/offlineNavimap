#include "route_mgr.hpp"
#include "map_engine.hpp"


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

    void RouteMgr::buildRoute(std::vector<m2::PointD>& points)
    {
        auto &routingSession = engine->getFramework()->GetRoutingManager().RoutingSession();
        routing::ReadyCallbackKsp onReady = [&,this](const std::vector<std::shared_ptr<routing::Route>>& routes,
                                           routing::RouterResultCode code)
        {
            int i = 0;
            if (code == routing::RouterResultCode::NoError) {
                std::vector<std::string> colors;
                colors.push_back("#ff0000");
                colors.push_back("#00ff00");
                colors.push_back("#0000ff");

                for(int i = 0; i < 3; i++){
                    auto wrapper = std::make_shared<RouteWrapper>(*this,*routes[i].get());
                    caculatedRoutes.push_back(wrapper);
                    wrapper.get()->show(colors[i],"#000000");
                }
//                caculatedRoutes.push_back( std::make_shared<RouteWrapper>(*this,route) );
//                showRoute(route,colors[i]);
//                i = (i+1)%3;
            }
        };
        auto onNeedMap = [&](uint64_t, std::set<std::string> const &) {

        };
        auto onRmRode = [&](routing::RouterResultCode code) {

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

    void RouteMgr::showRoute(const std::string& routeId)
    {
        auto routePtr = getRoute(routeId);
        if( routePtr == nullptr )
            return;
        routePtr.get()->show("","");
    }

    void RouteMgr::showRoute(routing::Route const& route, std::string color)
    {
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
        routeMgr(mgr),
        route(r)
    {

    }
    RouteWrapper::~RouteWrapper()
    {

    }

    void RouteWrapper::show(const std::string& fillColor, const std::string& outlineColor)
    {
        std::vector<routing::RouteSegment> segments;
        double distance = 0.0;
        auto const subroutesCount = route.GetSubrouteCount();

        df::DrapeEngineSafePtr drapeEngine;
        drapeEngine.Set(routeMgr.GetDrapeEngine());

        for (size_t subrouteIndex = route.GetCurrentSubrouteIdx(); subrouteIndex < subroutesCount; ++subrouteIndex) {
            route.GetSubrouteInfo(subrouteIndex, segments);

            auto const startPt = route.GetSubrouteAttrs(subrouteIndex).GetStart().GetPoint();
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
        return route.GetRouterId() == routeId;
    }


};
