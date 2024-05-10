#pragma once

#include <routing/base/ft_astar_ksp_bxj.hpp>
#include "../routing/index_router.hpp"

#include "../routing_common/ft_car_model_factory.hpp"

namespace routing
{
class FtIndexRouter : public IndexRouter
{
public:
  FtIndexRouter(VehicleType vehicleType, bool loadAltitudes,
                CountryParentNameGetterFn const & countryParentNameGetterFn,
                TCountryFileFn const & countryFileFn, CourntryRectFn const & countryRectFn,
                std::shared_ptr<NumMwmIds> numMwmIds,
                std::unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                traffic::TrafficCache const & trafficCache, DataSource & dataSource,
                FtStrategy strategy);

  RouterResultCode CalculateRouteKsp(Checkpoints const & checkpoints,
                                     m2::PointD const & startDirection, bool adjustToPrevRoute,
                                     RouterDelegate const & delegate,
                                     std::vector<std::shared_ptr<Route>> & vec_route,
                                     const int k) ;

private:
  RouterResultCode DoCalculateRouteKsp(Checkpoints const & checkpoints,
                                       m2::PointD const & startDirection,
                                       RouterDelegate const & delegate,
                                       std::vector<std::shared_ptr<Route>> & vec_route,
                                       const int k);

  RouterResultCode CalculateSubrouteKsp(Checkpoints const & checkpoints, size_t subrouteIdx,
                                        RouterDelegate const & delegate,
                                        std::shared_ptr<AStarProgress> const & progress,
                                        IndexGraphStarter & graph,
                                        std::vector<std::vector<Segment>> & vec_subroute,
                                        const int k, bool guidesActive = false);

  RouterResultCode CalculateSubrouteJointsModeKsp(IndexGraphStarter & starter,
                                                  RouterDelegate const & delegate,
                                                  std::shared_ptr<AStarProgress> const & progress,
                                                  std::vector<std::vector<Segment>> & subroute,
                                                  const int k);

  RouterResultCode MultiCalculateSubrouteJointsMode(
      IndexGraphStarter & starter, RouterDelegate const & delegate,
      std::shared_ptr<AStarProgress> const & progress,
      std::vector<std::vector<Segment>> & vec_subroute, const int k);

  RouterResultCode CalculateSubrouteJointsMode(IndexGraphStarter & starter,
                                               RouterDelegate const & delegate,
                                               std::shared_ptr<AStarProgress> const & progress,
                                               std::vector<Segment> & subroute,
                                               std::set<Full_Edge> & set_route_edge);

  //已求得的路径作为参数传进来
  template <typename Vertex, typename Edge, typename Weight, typename AStarParams>
  RouterResultCode FindPathKsp(AStarParams & params, std::set<NumMwmId> const & mwmIds,
                               RoutingResult<Vertex, Weight> & routingResult,
                               WorldGraphMode mode) const
  {
    AStarKspAlgorithm<Vertex, Edge, Weight> algorithm;
    return ConvertTransitResult(mwmIds, ConvertResult<Vertex, Edge, Weight>(
                                            algorithm.FindPathBidirectionalKsp(params, routingResult)));
  }
};
}  // namespace routing