// 此文件使用了白小军的思路，用于计算多条路径
// 在A*算法的基础上，计算出第一条路径之后，将已求得的路径的边的权重加大，再次计算求得新路径。
#pragma once

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_graph.hpp"
#include "routing/base/astar_vertex_data.hpp"
#include "routing/base/astar_weight.hpp"
#include "routing/base/routing_result.hpp"
#include "routing/joint_segment.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "../../3party/skarupke/bytell_hash_map.hpp"

extern double g_ksp_factor;

namespace routing
{
struct Full_Edge
{
  using Vertex = JointSegment;
  Vertex from;
  Vertex to;

  Full_Edge() = default;
  Full_Edge(const Vertex & from, const Vertex & to) : from(from), to(to) {}
  bool operator<(const Full_Edge & other) const { return from < other.from ? true : to < other.to; }
  bool operator==(const Full_Edge & other) const { return from == other.from && to == other.to; }
};

template <typename Vertex, typename Edge, typename Weight>
class AStarKspAlgorithm : public AStarAlgorithm<Vertex, Edge, Weight>
{
public:
  using Graph = typename AStarAlgorithm<Vertex, Edge, Weight>::Graph;
  using State = typename AStarAlgorithm<Vertex, Edge, Weight>::State;
  using Result = typename AStarAlgorithm<Vertex, Edge, Weight>::Result;
  using BidirectionalStepContext =
      typename AStarAlgorithm<Vertex, Edge, Weight>::BidirectionalStepContext;
  using PeriodicPollCancellable =
      typename AStarAlgorithm<Vertex, Edge, Weight>::PeriodicPollCancellable;

public:
  template <typename Visitor = astar::DefaultVisitor<Vertex>,
            typename LengthChecker = astar::DefaultLengthChecker<Weight>>
  struct ParamsKsp
    : public AStarAlgorithm<Vertex, Edge, Weight>::template Params<Visitor, LengthChecker>
  {
    ParamsKsp(Graph & graph, Vertex const & startVertex, Vertex const & finalVertex,
              std::vector<Edge> const * prevRoute, base::Cancellable const & cancellable,
              std::set<Full_Edge> & set_route_edge,
              Visitor && onVisitedVertexCallback = astar::DefaultVisitor<Vertex>(),
              LengthChecker && checkLengthCallback = astar::DefaultLengthChecker<Weight>())
      : AStarAlgorithm<Vertex, Edge, Weight>::template Params<Visitor, LengthChecker>(graph, startVertex, finalVertex, prevRoute, cancellable,
               std::forward<Visitor>(onVisitedVertexCallback),
               std::forward<LengthChecker>(checkLengthCallback))
      ,m_set_route_edge(set_route_edge)
    {
    }

    std::set<Full_Edge> & m_set_route_edge;
  };

public:
  template <typename P>
  typename AStarAlgorithm<Vertex, Edge, Weight>::Result FindPathBidirectional(
      P & params, RoutingResult<Vertex, Weight> & result,
      const std::set<Full_Edge> & set_full_edge) const;

  template <typename P>
  typename AStarAlgorithm<Vertex, Edge, Weight>::Result FindPathBidirectionalKsp(
      P & params, RoutingResult<Vertex, Weight> & result) const;

  // k: 想要规划 k 条路径
  // n: 结果规划 n 条路径
  template <typename P>
  typename AStarAlgorithm<Vertex, Edge, Weight>::Result FindPathKsp(
      P & params, std::vector<RoutingResult<Vertex, Weight>> & result, const int k) const;

  template <typename P>
  typename AStarAlgorithm<Vertex, Edge, Weight>::Result FindPathKsp(
      P & params, std::vector<RoutingResult<Vertex, Weight>> & result) const;

  //已求得的路径作为参数传进来
  // routing_result 只是一条路径
  template <typename P>
  typename AStarAlgorithm<Vertex, Edge, Weight>::Result FindPathKsp(
      P & params, RoutingResult<Vertex, Weight> & routing_result) const;
};

// 已求得的路径作为参数传进来
template <typename Vertex, typename Edge, typename Weight>
template <typename P>
typename AStarAlgorithm<Vertex, Edge, Weight>::Result
AStarKspAlgorithm<Vertex, Edge, Weight>::FindPathBidirectionalKsp(
    P & params, RoutingResult<Vertex, Weight> & result) const
{
  auto const epsilon = params.m_weightEpsilon;
  auto & graph = params.m_graph;
  auto const & finalVertex = params.m_finalVertex;
  auto const & startVertex = params.m_startVertex;

  BidirectionalStepContext forward(true /* forward */, startVertex, finalVertex, graph);
  BidirectionalStepContext backward(false /* forward */, startVertex, finalVertex, graph);

  auto & forwardParents = forward.GetParents();
  auto & backwardParents = backward.GetParents();

  bool foundAnyPath = false;
  auto bestPathReducedLength = AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance;
  auto bestPathRealLength = AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance;

  forward.UpdateDistance(State(startVertex, AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance));
  forward.queue.push(State(startVertex, AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance,
                           forward.ConsistentHeuristic(startVertex)));

  backward.UpdateDistance(State(finalVertex, AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance));
  backward.queue.push(State(finalVertex, AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance,
                            backward.ConsistentHeuristic(finalVertex)));

  // To use the search code both for backward and forward directions
  // we keep the pointers to everything related to the search in the
  // 'current' and 'next' directions. Swapping these pointers indicates
  // changing the end we are searching from.
  BidirectionalStepContext * cur = &forward;
  BidirectionalStepContext * nxt = &backward;

  auto const getResult = [&]()
  {
    if (!params.m_checkLengthCallback(bestPathRealLength))
      return Result::NoPath;

    AStarAlgorithm<Vertex, Edge, Weight>::ReconstructPathBidirectional(
        cur->bestVertex, nxt->bestVertex, cur->parent, nxt->parent, result.m_path);
    result.m_distance = bestPathRealLength;
    CHECK(!result.m_path.empty(), ());
    if (!cur->forward)
      reverse(result.m_path.begin(), result.m_path.end());

    return Result::OK;
  };

  std::vector<Edge> adj;

  // It is not necessary to check emptiness for both queues here
  // because if we have not found a path by the time one of the
  // queues is exhausted, we never will.
  uint32_t steps = 0;
  PeriodicPollCancellable periodicCancellable(params.m_cancellable);

  while (!cur->queue.empty() && !nxt->queue.empty())
  {
    ++steps;

    if (periodicCancellable.IsCancelled())
      return Result::Cancelled;

    if (steps % AStarAlgorithm<Vertex, Edge, Weight>::kQueueSwitchPeriod == 0)
      std::swap(cur, nxt);

    if (foundAnyPath)
    {
      auto const curTop = cur->TopDistance();
      auto const nxtTop = nxt->TopDistance();

      // The intuition behind this is that we cannot obtain a path shorter
      // than the left side of the inequality because that is how any path we find
      // will look like (see comment for curPathReducedLength below).
      // We do not yet have the proof that we will not miss a good path by doing so.

      // The shortest reduced path corresponds to the shortest real path
      // because the heuristic we use are consistent.
      // It would be a mistake to make a decision based on real path lengths because
      // several top states in a priority queue may have equal reduced path lengths and
      // different real path lengths.

      if (curTop + nxtTop >= bestPathReducedLength - epsilon)
        return getResult();
    }

    State const stateV = cur->queue.top();
    cur->queue.pop();

    if (cur->ExistsStateWithBetterDistance(stateV))
      continue;

    params.m_onVisitedVertexCallback(stateV.vertex,
                                     cur->forward ? cur->finalVertex : cur->startVertex);

    cur->GetAdjacencyList(stateV, adj);
    auto const & pV = stateV.heuristic;
    for (auto const & edge : adj)
    {
      auto weight = edge.GetWeight();

      // 对于已找到路径的边，计算权重时，要乘以一个系数(规避因子)，以降低它在新路径中被使用的机会。
      Full_Edge full_edge(stateV.vertex, edge.GetTarget());
      if (params.m_set_route_edge.find(full_edge) != params.m_set_route_edge.end())
      {
        //weight = g_ksp_factor*weight;
        weight = 10.0*weight;
      }

      State stateW(edge.GetTarget(), AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance);

      if (stateV.vertex == stateW.vertex)
        continue;

      auto const pW = cur->ConsistentHeuristic(stateW.vertex);
      auto const reducedWeight = weight + pW - pV;

      CHECK_GREATER_OR_EQUAL(
          reducedWeight, -epsilon,
          ("Invariant violated for:", "v =", stateV.vertex, "w =", stateW.vertex));

      stateW.distance =
          stateV.distance +
          std::max(reducedWeight, AStarAlgorithm<Vertex, Edge, Weight>::kZeroDistance);

      auto const fullLength = weight + stateV.distance + cur->pS - pV;
      if (!params.m_checkLengthCallback(fullLength))
        continue;

      if (cur->ExistsStateWithBetterDistance(stateW, epsilon))
        continue;

      stateW.heuristic = pW;
      cur->UpdateDistance(stateW);
      cur->UpdateParent(stateW.vertex, stateV.vertex);

      if (auto op = nxt->GetDistance(stateW.vertex); op)
      {
        auto const & distW = *op;
        // Reduced length that the path we've just found has in the original graph:
        // find the reduced length of the path's parts in the reduced forward and backward graphs.
        auto const curPathReducedLength = stateW.distance + distW;
        // No epsilon here: it is ok to overshoot slightly.
        if ((!foundAnyPath || bestPathReducedLength > curPathReducedLength) &&
            graph.AreWavesConnectible(forwardParents, stateW.vertex, backwardParents))
        {
          bestPathReducedLength = curPathReducedLength;

          bestPathRealLength = stateV.distance + weight + distW;
          bestPathRealLength += cur->pS - pV;
          bestPathRealLength += nxt->pS - nxt->ConsistentHeuristic(stateW.vertex);

          foundAnyPath = true;
          cur->bestVertex = stateV.vertex;
          nxt->bestVertex = stateW.vertex;
        }
      }

      if (stateW.vertex != (cur->forward ? cur->finalVertex : cur->startVertex))
        cur->queue.push(stateW);
    }
  }

  if (foundAnyPath)
    return getResult();

  return Result::NoPath;
}

}  // namespace routing
