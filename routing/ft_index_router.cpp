#include "ft_index_router.hpp"

#include "../routing/base/astar_progress.hpp"
#include "../routing/junction_visitor.hpp"
#include "../routing/routing_helpers.hpp"

#include "../routing_common/bicycle_model.hpp"
#include "../routing_common/pedestrian_model.hpp"

#include "../base/scope_guard.hpp"

#include <memory>

namespace
{
using namespace routing;
using namespace std;

// kAdjustRangeM 和 kMinDistanceToFinishM 在index_router.cpp 的匿名命名空间中定义过
// If user left the route within this range(meters), adjust the route. Else full rebuild.
double constexpr kAdjustRangeM = 5000.0;
// Full rebuild if distance(meters) is less.
double constexpr kMinDistanceToFinishM = 10000;

uint32_t constexpr kVisitPeriod = 40;
double constexpr kAlmostZeroContribution = 1e-7;

shared_ptr<VehicleModelFactoryInterface> CreateVehicleModelFactory(
    VehicleType vehicleType, CountryParentNameGetterFn const & countryParentNameGetterFn,
    FtStrategy strategy)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit: return make_shared<PedestrianModelFactory>(countryParentNameGetterFn);
  case VehicleType::Bicycle: return make_shared<BicycleModelFactory>(countryParentNameGetterFn);
  case VehicleType::Car: return make_shared<FtCarModelFactory>(countryParentNameGetterFn, strategy);
  case VehicleType::Count:
  default:
    CHECK(false, ("Can't create VehicleModelFactoryInterface for", vehicleType));
    return nullptr;
  }
  UNREACHABLE();
}

}  // namespace

namespace routing
{

vector<Segment> ProcessJoints(vector<JointSegment> const & jointsPath,
                              IndexGraphStarterJoints<IndexGraphStarter> & jointStarter);


FtIndexRouter::FtIndexRouter(VehicleType vehicleType, bool loadAltitudes,
                             CountryParentNameGetterFn const & countryParentNameGetterFn,
                             TCountryFileFn const & countryFileFn,
                             CourntryRectFn const & countryRectFn,
                             std::shared_ptr<NumMwmIds> numMwmIds,
                             std::unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                             traffic::TrafficCache const & trafficCache, DataSource & dataSource,
                             FtStrategy strategy)
  : IndexRouter(vehicleType, loadAltitudes,
                CreateVehicleModelFactory(vehicleType, countryParentNameGetterFn, strategy),
                countryParentNameGetterFn, countryFileFn, countryRectFn, std::move(numMwmIds),
                std::move(numMwmTree), trafficCache, dataSource)
{
}

RouterResultCode FtIndexRouter::CalculateRouteKsp(
    Checkpoints const & checkpoints, m2::PointD const & startDirection, bool adjustToPrevRoute,
    RouterDelegate const & delegate, std::vector<std::shared_ptr<Route>> & vec_route, const int k)
{
  auto const & startPoint = checkpoints.GetStart();
  auto const & finalPoint = checkpoints.GetFinish();

  try
  {
    SCOPE_GUARD(featureRoadGraphClear, [this] { this->ClearState(); });

    if (adjustToPrevRoute && m_lastRoute && m_lastFakeEdges &&
        finalPoint == m_lastRoute->GetFinish())
    {
      double const distanceToRoute = m_lastRoute->CalcDistance(startPoint);
      double const distanceToFinish = mercator::DistanceOnEarth(startPoint, finalPoint);
      if (distanceToRoute <= kAdjustRangeM && distanceToFinish >= kMinDistanceToFinishM)
      {
        for (auto route : vec_route)
        {
          auto const code = AdjustRoute(checkpoints, startDirection, delegate, *route);
          if (code != RouterResultCode::RouteNotFound)
            return code;

          LOG(LWARNING, ("Can't adjust route, do full rebuild, prev start:",
                         mercator::ToLatLon(m_lastRoute->GetStart()),
                         ", start:", mercator::ToLatLon(startPoint),
                         ", finish:", mercator::ToLatLon(finalPoint)));
        }
      }
    }

    VehicleModelFactoryInterface & vehicleModelFactory = *m_vehicleModelFactory;
    vehicleModelFactory.SetAvoidPolygon(m_polygons);

    return DoCalculateRouteKsp(checkpoints, startDirection, delegate, vec_route, k);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't find path from", mercator::ToLatLon(startPoint), "to",
                 mercator::ToLatLon(finalPoint), ":\n ", e.what()));
    return RouterResultCode::InternalError;
  }
}

RouterResultCode FtIndexRouter::DoCalculateRouteKsp(Checkpoints const & checkpoints,
                                                    m2::PointD const & startDirection,
                                                    RouterDelegate const & delegate,
                                                    std::vector<std::shared_ptr<Route>> & vec_route,
                                                    const int k)
{
  m_lastRoute.reset();
  // MwmId used for guides segments in RedressRoute().
  NumMwmId guidesMwmId = kFakeNumMwmId;

  for (auto const & checkpoint : checkpoints.GetPoints())
  {
    string const countryName = m_countryFileFn(checkpoint);

    if (countryName.empty())
    {
      LOG(LWARNING, ("For point", mercator::ToLatLon(checkpoint),
                     "CountryInfoGetter returns an empty CountryFile(). It happens when checkpoint"
                     "is put at gaps between mwm."));
      return RouterResultCode::InternalError;
    }

    auto const country = platform::CountryFile(countryName);
    if (!m_dataSource.IsLoaded(country))
    {
      vec_route[0]->AddAbsentCountry(country.GetName());
    }
    else if (guidesMwmId == kFakeNumMwmId)
    {
      guidesMwmId = m_numMwmIds->GetId(country);
    };
  }

  if (!vec_route[0]->GetAbsentCountries().empty())
    return RouterResultCode::NeedMoreMaps;

  TrafficStash::Guard guard(m_trafficStash);
  unique_ptr<WorldGraph> graph = MakeWorldGraph();

  vector<vector<Segment>> vec_segments(k);

  vector<Segment> startSegments;
  bool startSegmentIsAlmostCodirectionalDirection = false;
  bool const foundStart =
      FindBestSegments(checkpoints.GetPointFrom(), startDirection, true /* isOutgoing */, *graph,
                       startSegments, startSegmentIsAlmostCodirectionalDirection);

  m_guides.SetGuidesGraphParams(guidesMwmId, m_estimator->GetMaxWeightSpeedMpS());
  m_guides.ConnectToGuidesGraph(checkpoints.GetPoints());

  if (!m_guides.IsActive() && !foundStart)
  {
    return RouterResultCode::StartPointNotFound;
  }

  uint32_t const startIdx = ConnectTracksOnGuidesToOsm(checkpoints.GetPoints(), *graph);
  ConnectCheckpointsOnGuidesToOsm(checkpoints.GetPoints(), *graph);

  vector<size_t> vec_subrouteSegmentsBegin(k, 0);
  vector<vector<Route::SubrouteAttrs>> vec_subroutes(k);
  for (int i = 0; i < k; i++)
  {
    routing_ksp_help::PushPassedSubroutes_help(checkpoints, vec_subroutes[i]);
  }

  unique_ptr<IndexGraphStarter> starter;

  auto progress = make_shared<AStarProgress>();
  double const checkpointsLength = checkpoints.GetSummaryLengthBetweenPointsMeters();

  for (size_t i = checkpoints.GetPassedIdx(); i < checkpoints.GetNumSubroutes(); ++i)
  {
    bool const isFirstSubroute = i == checkpoints.GetPassedIdx();
    bool const isLastSubroute = i == checkpoints.GetNumSubroutes() - 1;
    auto const & startCheckpoint = checkpoints.GetPoint(i);
    auto const & finishCheckpoint = checkpoints.GetPoint(i + 1);

    FakeEnding finishFakeEnding = m_guides.GetFakeEnding(i + 1);

    vector<Segment> finishSegments;
    bool dummy = false;

    // Stop building route if |finishCheckpoint| is not connected to OSM and is not connected to
    // the guides graph.
    if (!FindBestSegments(finishCheckpoint, m2::PointD::Zero() /* direction */,
                          false /* isOutgoing */, *graph, finishSegments,
                          dummy /* bestSegmentIsAlmostCodirectional */) &&
        finishFakeEnding.m_projections.empty())
    {
      return isLastSubroute ? RouterResultCode::EndPointNotFound
                            : RouterResultCode::IntermediatePointNotFound;
    }

    bool isStartSegmentStrictForward = (m_vehicleType == VehicleType::Car);
    if (isFirstSubroute)
      isStartSegmentStrictForward = startSegmentIsAlmostCodirectionalDirection;

    FakeEnding startFakeEnding = m_guides.GetFakeEnding(i);

    if (startFakeEnding.m_projections.empty())
      startFakeEnding = MakeFakeEnding(startSegments, startCheckpoint, *graph);

    if (finishFakeEnding.m_projections.empty())
      finishFakeEnding = MakeFakeEnding(finishSegments, finishCheckpoint, *graph);

    uint32_t const fakeNumerationStart =
        starter ? starter->GetNumFakeSegments() + startIdx : startIdx;
    IndexGraphStarter subrouteStarter(startFakeEnding, finishFakeEnding, fakeNumerationStart,
                                      isStartSegmentStrictForward, *graph);

    if (m_guides.IsAttached())
    {
      subrouteStarter.SetGuides(m_guides.GetGuidesGraph());
      AddGuidesOsmConnectionsToGraphStarter(i, i + 1, subrouteStarter);
    }

    vector<vector<Segment>> vec_subroute;
    double contributionCoef = kAlmostZeroContribution;
    if (!base::AlmostEqualAbs(checkpointsLength, 0.0, 1e-5))
    {
      contributionCoef =
          mercator::DistanceOnEarth(startCheckpoint, finishCheckpoint) / checkpointsLength;
    }

    AStarSubProgress subProgress(mercator::ToLatLon(startCheckpoint),
                                 mercator::ToLatLon(finishCheckpoint), contributionCoef);
    progress->AppendSubProgress(subProgress);
    SCOPE_GUARD(eraseProgress, [&progress]() { progress->PushAndDropLastSubProgress(); });

    auto const result = CalculateSubrouteKsp(checkpoints, i, delegate, progress, subrouteStarter,
                                             vec_subroute, k, m_guides.IsAttached());

    if (result != RouterResultCode::NoError)
      return result;

    int n = vec_subroute.size();

    for (int i = 0; i < k; i++)
    {
      IndexGraphStarter::CheckValidRoute(vec_subroute[i < n ? i : 0]);
      vec_segments[i].insert(vec_segments[i].end(), vec_subroute[i < n ? i : 0].begin(),
                             vec_subroute[i < n ? i : 0].end());
      size_t subrouteSegmentsEnd = vec_segments[i].size();
      vec_subroutes[i].emplace_back(subrouteStarter.GetStartJunction().ToPointWithAltitude(),
                                    subrouteStarter.GetFinishJunction().ToPointWithAltitude(),
                                    vec_subrouteSegmentsBegin[i], subrouteSegmentsEnd);
      vec_subrouteSegmentsBegin[i] = subrouteSegmentsEnd;
    }

    // For every subroute except for the first one the last real segment is used  as a start
    // segment. It's implemented this way to prevent jumping from one road to another one using a
    // via point.
    startSegments.resize(1);
    bool const hasRealOrPart = routing_ksp_help::GetLastRealOrPart_help(
        subrouteStarter, vec_subroute[0], startSegments[0]);
    CHECK(hasRealOrPart, ("No real or part of real segments in route."));
    if (!starter)
      starter = make_unique<IndexGraphStarter>(move(subrouteStarter));
    else
      starter->Append(FakeEdgesContainer(move(subrouteStarter)));
  }

  for (int i = 0; i < k; i++)
  {
    vec_route[i]->SetCurrentSubrouteIdx(checkpoints.GetPassedIdx());
    vec_route[i]->SetSubroteAttrs(move(vec_subroutes[i]));
    IndexGraphStarter::CheckValidRoute(vec_segments[i]);
    auto redressResult =
        RedressRoute(vec_segments[i], delegate.GetCancellable(), *starter, *vec_route[i]);
    if (redressResult != RouterResultCode::NoError)
      return redressResult;
    LOG(LINFO, ("Route length:", vec_route[i]->GetTotalDistanceMeters(),
                "meters. ETA:", vec_route[i]->GetTotalTimeSec(), "seconds."));
  }

  // 不知道 m_lastRoute 做什么用的，先不管它。
  // m_lastRoute = make_unique<SegmentedRoute>(checkpoints.GetStart(), checkpoints.GetFinish(),
  //                                           route.GetSubroutes());
  // for (Segment const & segment : segments)
  //   m_lastRoute->AddStep(segment,
  //                        mercator::FromLatLon(starter->GetPoint(segment, true /* front */)));

  // m_lastFakeEdges = make_unique<FakeEdgesContainer>(move(*starter));

  return RouterResultCode::NoError;
}

RouterResultCode FtIndexRouter::CalculateSubrouteKsp(
    Checkpoints const & checkpoints, size_t subrouteIdx, RouterDelegate const & delegate,
    shared_ptr<AStarProgress> const & progress, IndexGraphStarter & starter,
    vector<vector<Segment>> & vec_subroute, const int k, bool guidesActive /* = false */)
{
  CHECK(progress, (checkpoints));
  vec_subroute.clear();

  SetupAlgorithmMode(starter, guidesActive);
  LOG(LINFO, ("Routing in mode:", starter.GetGraph().GetMode()));

  base::ScopedTimerWithLog timer("Route build");
  WorldGraphMode const mode = starter.GetGraph().GetMode();
  switch (mode)
  {
  case WorldGraphMode::Joints:
    // return CalculateSubrouteJointsModeKsp(starter, delegate, progress, vec_subroute, k);
    return MultiCalculateSubrouteJointsMode(starter, delegate, progress, vec_subroute, k);

    // 不知道做什么用的，好像没有执行到，先注释了试试。
  // case WorldGraphMode::NoLeaps:
  //   return CalculateSubrouteNoLeapsMode(starter, delegate, progress, subroute);
  // case WorldGraphMode::LeapsOnly:
  //   return CalculateSubrouteLeapsOnlyMode(checkpoints, subrouteIdx, starter, delegate, progress,
  //                                         subroute);
  default: CHECK(false, ("Wrong WorldGraphMode here:", mode));
  }
  UNREACHABLE();
}

RouterResultCode FtIndexRouter::MultiCalculateSubrouteJointsMode(
    IndexGraphStarter & starter, RouterDelegate const & delegate,
    shared_ptr<AStarProgress> const & progress, vector<vector<Segment>> & vec_subroute, const int k)
{
  RouterResultCode result_code = RouterResultCode::NoError;
  std::set<Full_Edge> set_route_edge;  //已找到路径的边的集合
  for (int i = 0; i < k; i++)
  {
    vector<Segment> subroute;
    result_code =
        CalculateSubrouteJointsMode(starter, delegate, progress, subroute, set_route_edge);
    if (result_code != RouterResultCode::NoError)
    {
      return result_code;
    }
    vec_subroute.push_back(subroute);
  }
  return result_code;
}

RouterResultCode FtIndexRouter::CalculateSubrouteJointsMode(
    IndexGraphStarter & starter, RouterDelegate const & delegate,
    shared_ptr<AStarProgress> const & progress, vector<Segment> & subroute,
    std::set<Full_Edge> & set_route_edge)
{
  using JointsStarter = IndexGraphStarterJoints<IndexGraphStarter>;
  JointsStarter jointStarter(starter, starter.GetStartSegment(), starter.GetFinishSegment());

  using Visitor = JunctionVisitor<JointsStarter>;
  Visitor visitor(jointStarter, delegate, kVisitPeriod, progress);

  using Vertex = JointsStarter::Vertex;
  using Edge = JointsStarter::Edge;
  using Weight = JointsStarter::Weight;

  AStarKspAlgorithm<Vertex, Edge, Weight>::ParamsKsp<Visitor, AStarLengthChecker> params(
      jointStarter, jointStarter.GetStartJoint(), jointStarter.GetFinishJoint(),
      nullptr /* prevRoute */, delegate.GetCancellable(), set_route_edge, move(visitor),
      AStarLengthChecker(starter));

  RoutingResult<Vertex, Weight> routingResult;
  RouterResultCode const result = FindPathKsp<Vertex, Edge, Weight>(
      params, {} /* mwmIds */, routingResult, WorldGraphMode::Joints);

  if (result != RouterResultCode::NoError)
    return result;

  for (size_t j = 0; j < routingResult.m_path.size() - 1; j++)
  {
    Full_Edge full_edge(routingResult.m_path[j], routingResult.m_path[j + 1]);
    Full_Edge full_edge2(routingResult.m_path[j+1], routingResult.m_path[j]);
    set_route_edge.insert(full_edge);
    set_route_edge.insert(full_edge2);
  }
  subroute = ProcessJoints(routingResult.m_path, jointStarter);
  return result;
}

}  // namespace routing