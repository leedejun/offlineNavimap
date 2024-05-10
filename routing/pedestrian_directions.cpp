#include "routing/pedestrian_directions.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns_generator.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <utility>

using namespace std;

namespace
{
    bool HasType(uint32_t type, feature::TypesHolder const & types)
    {
        for (uint32_t t : types)
        {
            t = ftypes::BaseChecker::PrepareToMatch(t, 2);
            if (type == t)
                return true;
        }
        return false;
    }

    using namespace routing;
    using namespace routing::turns;

    class RoutingResult : public IRoutingResult
    {
    public:
        RoutingResult(IRoadGraph::EdgeVector const & routeEdges,
                      PedestrianDirectionsEngine::AdjacentEdgesMap const & adjacentEdges,
                      TUnpackedPathSegments const & pathSegments)
                : m_routeEdges(routeEdges)
                , m_adjacentEdges(adjacentEdges)
                , m_pathSegments(pathSegments)
                , m_routeLength(0)
        {
            for (auto const & edge : routeEdges)
            {
                m_routeLength += mercator::DistanceOnEarth(edge.GetStartJunction().GetPoint(),
                                                           edge.GetEndJunction().GetPoint());
            }
        }

        // turns::IRoutingResult overrides:
        TUnpackedPathSegments const & GetSegments() const override { return m_pathSegments; }

        void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint,
                              size_t & ingoingCount, TurnCandidates & outgoingTurns) const override
        {
            CHECK(!segmentRange.IsEmpty(), ("SegmentRange presents a fake feature.",
                    "junctionPoint:", mercator::ToLatLon(junctionPoint)));

            ingoingCount = 0;
            outgoingTurns.candidates.clear();

            auto const adjacentEdges = m_adjacentEdges.find(segmentRange);
            if (adjacentEdges == m_adjacentEdges.cend())
            {
                ASSERT(false, ());
                return;
            }

            ingoingCount = adjacentEdges->second.m_ingoingTurnsCount;
            outgoingTurns = adjacentEdges->second.m_outgoingTurns;
        }

        double GetPathLength() const override { return m_routeLength; }

        geometry::PointWithAltitude GetStartPoint() const override
        {
            CHECK(!m_routeEdges.empty(), ());
            return m_routeEdges.front().GetStartJunction();
        }

        geometry::PointWithAltitude GetEndPoint() const override
        {
            CHECK(!m_routeEdges.empty(), ());
            return m_routeEdges.back().GetEndJunction();
        }

    private:
        IRoadGraph::EdgeVector const & m_routeEdges;
        PedestrianDirectionsEngine::AdjacentEdgesMap const & m_adjacentEdges;
        TUnpackedPathSegments const & m_pathSegments;
        double m_routeLength;
    };

/// \brief This method should be called for an internal junction of the route with corresponding
/// |ingoingEdges|, |outgoingEdges|, |ingoingRouteEdge| and |outgoingRouteEdge|.
/// \returns false if the junction is an internal point of feature segment and can be considered as
/// a part of LoadedPathSegment and returns true if the junction should be considered as a beginning
/// of a new LoadedPathSegment.
    bool IsJoint(IRoadGraph::EdgeVector const & ingoingEdges,
                 IRoadGraph::EdgeVector const & outgoingEdges, Edge const & ingoingRouteEdge,
                 Edge const & outgoingRouteEdge, bool isCurrJunctionFinish, bool isInEdgeReal)
    {
        // When feature id is changed at a junction this junction should be considered as a joint.
        //
        // If a feature id is not changed at a junction but the junction has some ingoing or outgoing
        // edges with different feature ids, the junction should be considered as a joint.
        //
        // If a feature id is not changed at a junction and all ingoing and outgoing edges of the junction
        // has the same feature id, the junction still may be considered as a joint. It happens in case of
        // self intersected features. For example:
        //            *--Seg3--*
        //            |        |
        //          Seg4      Seg2
        //            |        |
        //   *--Seg0--*--Seg1--*
        // The common point of segments 0, 1 and 4 should be considered as a joint.
        if (!isInEdgeReal)
            return true;

        if (isCurrJunctionFinish)
            return true;

        if (ingoingRouteEdge.GetFeatureId() != outgoingRouteEdge.GetFeatureId())
            return true;

        FeatureID const & featureId = ingoingRouteEdge.GetFeatureId();
        uint32_t const segOut = outgoingRouteEdge.GetSegId();
        for (Edge const & e : ingoingEdges)
        {
            if (e.GetFeatureId() != featureId || abs(static_cast<int32_t>(segOut - e.GetSegId())) != 1)
                return true;
        }

        uint32_t const segIn = ingoingRouteEdge.GetSegId();
        for (Edge const & e : outgoingEdges)
        {
            // It's necessary to compare segments for cases when |featureId| is a loop.
            if (e.GetFeatureId() != featureId || abs(static_cast<int32_t>(segIn - e.GetSegId())) != 1)
                return true;
        }
        return false;
    }
}  // namespace

namespace routing
{

    PedestrianDirectionsEngine::PedestrianDirectionsEngine(DataSource const & dataSource,
                                                           shared_ptr<NumMwmIds> numMwmIds)
            : m_typeSteps(classif().GetTypeByPath({"highway", "steps"}))
            , m_typeLiftGate(classif().GetTypeByPath({"barrier", "lift_gate"}))
            , m_typeGate(classif().GetTypeByPath({"barrier", "gate"}))
            , m_numMwmIds(move(numMwmIds))
            , m_dataSource(dataSource)
    {
    }

// PedestrianDirectionsEngine::PedestrianDirectionsEngine(shared_ptr<NumMwmIds> numMwmIds)
//   : m_typeSteps(classif().GetTypeByPath({"highway", "steps"}))
//   , m_typeLiftGate(classif().GetTypeByPath({"barrier", "lift_gate"}))
//   , m_typeGate(classif().GetTypeByPath({"barrier", "gate"}))
//   , m_numMwmIds(move(numMwmIds))
// {
// }

    bool PedestrianDirectionsEngine::Generate(IndexRoadGraph const & graph,
                                              vector<geometry::PointWithAltitude> const & path,
                                              base::Cancellable const & cancellable,
                                              Route::TTurns & turns, Route::TStreets & streetNames,
                                              vector<geometry::PointWithAltitude> & routeGeometry,
                                              vector<Segment> & segments)
    {
        turns.clear();
        streetNames.clear();
        segments.clear();
        // routeGeometry = path;
        routeGeometry.clear();

        size_t const pathSize = path.size();
        // Note. According to Route::IsValid() method route of zero or one point is invalid.
        if (pathSize <= 1)
            return false;

        vector<Edge> routeEdges;
        graph.GetRouteEdges(routeEdges);

        if (routeEdges.empty())
            return false;

        if (cancellable.IsCancelled())
            return false;

        FillPathSegmentsAndAdjacentEdgesMap(graph, path, routeEdges, cancellable);

        if (cancellable.IsCancelled())
            return false;

        ::RoutingResult resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
        auto const res = MakeTurnAnnotation(resultGraph, *m_numMwmIds, cancellable, routeGeometry, turns,
                                            streetNames, segments);

        if (res != RouterResultCode::NoError)
            return false;

        CHECK_EQUAL(
                routeGeometry.size(), pathSize,
                ("routeGeometry and path have different sizes. routeGeometry size:", routeGeometry.size(),
                        "path size:", pathSize, "segments size:", segments.size(), "routeEdges size:",
                        routeEdges.size(), "resultGraph.GetSegments() size:", resultGraph.GetSegments().size()));

        // In case of bicycle routing |m_pathSegments| may have an empty
        // |LoadedPathSegment::m_segments| fields. In that case |segments| is empty
        // so size of |segments| is not equal to size of |routeEdges|.
        if (!segments.empty())
            CHECK_EQUAL(segments.size(), routeEdges.size(), ());

        // CalculateTurns(graph, routeEdges, turns, cancellable);

        // graph.GetRouteSegments(segments);
        return true;
    }

    void PedestrianDirectionsEngine::FillPathSegmentsAndAdjacentEdgesMap(
            IndexRoadGraph const & graph, vector<geometry::PointWithAltitude> const & path,
            IRoadGraph::EdgeVector const & routeEdges, base::Cancellable const & cancellable)
    {
        m_pathSegments.clear();
        size_t const pathSize = path.size();
        CHECK_GREATER(pathSize, 1, ());
        CHECK_EQUAL(routeEdges.size() + 1, pathSize, ());
        // Filling |m_adjacentEdges|.
        auto constexpr kInvalidSegId = numeric_limits<uint32_t>::max();
        // |startSegId| is a value to keep start segment id of a new instance of LoadedPathSegment.
        uint32_t startSegId = kInvalidSegId;
        vector<geometry::PointWithAltitude> prevJunctions;
        vector<Segment> prevSegments;
        for (size_t i = 1; i < pathSize; ++i)
        {
            if (cancellable.IsCancelled())
                return;

            geometry::PointWithAltitude const & prevJunction = path[i - 1];
            geometry::PointWithAltitude const & currJunction = path[i];

            IRoadGraph::EdgeVector outgoingEdges;
            IRoadGraph::EdgeVector ingoingEdges;
            bool const isCurrJunctionFinish = (i + 1 == pathSize);
            GetEdges(graph, currJunction, isCurrJunctionFinish, outgoingEdges, ingoingEdges);

            Edge const & inEdge = routeEdges[i - 1];
            // Note. |inFeatureId| may be invalid in case of adding fake features.
            // It happens for example near starts and a finishes.
            FeatureID const & inFeatureId = inEdge.GetFeatureId();
            uint32_t const inSegId = inEdge.GetSegId();

            if (startSegId == kInvalidSegId)
                startSegId = inSegId;

            prevJunctions.push_back(prevJunction);
            prevSegments.push_back(ConvertEdgeToSegment(*m_numMwmIds, inEdge));

            if (!IsJoint(ingoingEdges, outgoingEdges, inEdge, routeEdges[i], isCurrJunctionFinish,
                         inFeatureId.IsValid()))
            {
                continue;
            }

            CHECK_EQUAL(prevJunctions.size(),
                        static_cast<size_t>(abs(static_cast<int32_t>(inSegId - startSegId)) + 1), ());

            prevJunctions.push_back(currJunction);

            AdjacentEdges adjacentEdges(ingoingEdges.size());
            SegmentRange segmentRange;
            GetSegmentRangeAndAdjacentEdges(outgoingEdges, inEdge, startSegId, inSegId, segmentRange,
                                            adjacentEdges.m_outgoingTurns);

            size_t const prevJunctionSize = prevJunctions.size();
            LoadedPathSegment pathSegment;
            LoadPathAttributes(segmentRange.GetFeature(), pathSegment);
            pathSegment.m_segmentRange = segmentRange;
            pathSegment.m_path = move(prevJunctions);
            // @TODO(bykoianko) |pathSegment.m_weight| should be filled here.

            // |prevSegments| contains segments which corresponds to road edges between joints. In case of a
            // fake edge a fake segment is created.
            CHECK_EQUAL(prevSegments.size() + 1, prevJunctionSize, ());
            pathSegment.m_segments = move(prevSegments);

            if (!segmentRange.IsEmpty())
            {
                auto const it = m_adjacentEdges.find(segmentRange);
                m_adjacentEdges.insert(it, make_pair(segmentRange, move(adjacentEdges)));
            }

            m_pathSegments.push_back(move(pathSegment));

            prevJunctions.clear();
            prevSegments.clear();
            startSegId = kInvalidSegId;
        }
    }

    void PedestrianDirectionsEngine::GetSegmentRangeAndAdjacentEdges(
            IRoadGraph::EdgeVector const & outgoingEdges, Edge const & inEdge, uint32_t startSegId,
            uint32_t endSegId, SegmentRange & segmentRange, TurnCandidates & outgoingTurns)
    {
        outgoingTurns.isCandidatesAngleValid = true;
        outgoingTurns.candidates.reserve(outgoingEdges.size());
        segmentRange = SegmentRange(inEdge.GetFeatureId(), startSegId, endSegId, inEdge.IsForward(),
                                    inEdge.GetStartPoint(), inEdge.GetEndPoint());
        CHECK(segmentRange.IsCorrect(), ());
        m2::PointD const & ingoingPoint = inEdge.GetStartJunction().GetPoint();
        m2::PointD const & junctionPoint = inEdge.GetEndJunction().GetPoint();

        for (auto const & edge : outgoingEdges)
        {
            if (edge.IsFake())
                continue;

            auto const & outFeatureId = edge.GetFeatureId();
            if (FakeFeatureIds::IsGuidesFeature(outFeatureId.m_index))
                continue;


            unique_ptr<FeatureType> ft;

            ft = GetLoader(outFeatureId.m_mwmId).GetFeatureByIndex(outFeatureId.m_index);

            if (!ft)
                continue;

            auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(*ft));
            ASSERT_NOT_EQUAL(
                    highwayClass, ftypes::HighwayClass::Error,
                    (mercator::ToLatLon(edge.GetStartPoint()), mercator::ToLatLon(edge.GetEndPoint())));
            ASSERT_NOT_EQUAL(
                    highwayClass, ftypes::HighwayClass::Undefined,
                    (mercator::ToLatLon(edge.GetStartPoint()), mercator::ToLatLon(edge.GetEndPoint())));

            bool const isLink = ftypes::IsLinkChecker::Instance()(*ft);

            double angle = 0;

            if (inEdge.GetFeatureId().m_mwmId == edge.GetFeatureId().m_mwmId)
            {
                if (mercator::DistanceOnEarth(junctionPoint, edge.GetStartJunction().GetPoint()) >=
                    turns::kFeaturesNearTurnMeters)
                    continue;
                ASSERT_LESS(mercator::DistanceOnEarth(junctionPoint, edge.GetStartJunction().GetPoint()),
                            turns::kFeaturesNearTurnMeters, ());
                m2::PointD const & outgoingPoint = edge.GetEndJunction().GetPoint();
                angle = base::RadToDeg(
                        turns::PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
            }
            else
            {
                // Note. In case of crossing mwm border
                // (inEdge.GetFeatureId().m_mwmId != edge.GetFeatureId().m_mwmId)
                // twins of inEdge.GetFeatureId() are considered as outgoing features.
                // In this case that turn candidate angle is invalid and
                // should not be used for turn generation.
                outgoingTurns.isCandidatesAngleValid = false;
            }
            outgoingTurns.candidates.emplace_back(angle, ConvertEdgeToSegment(*m_numMwmIds, edge),
                                                  highwayClass, isLink);
        }

        if (outgoingTurns.isCandidatesAngleValid)
            sort(outgoingTurns.candidates.begin(), outgoingTurns.candidates.end(),
                 base::LessBy(&TurnCandidate::m_angle));
    }

    void PedestrianDirectionsEngine::CalculateTurns(IndexRoadGraph const & graph,
                                                    vector<Edge> const & routeEdges,
                                                    Route::TTurns & turns,
                                                    base::Cancellable const & cancellable) const
    {
        for (size_t i = 0; i < routeEdges.size(); ++i)
        {
            if (cancellable.IsCancelled())
                return;

            Edge const & edge = routeEdges[i];

            if (edge.IsFake())
            {
                continue;
            }


            unique_ptr<FeatureType> ft;

            feature::TypesHolder types;
            graph.GetEdgeTypes(edge, types);

            if (HasType(m_typeSteps, types))
            {
                if (edge.IsForward())
                    turns.emplace_back(i, turns::PedestrianDirection::Upstairs);
                else
                    turns.emplace_back(i, turns::PedestrianDirection::Downstairs);
            }
            else
            {
                graph.GetJunctionTypes(edge.GetStartJunction(), types);

                // @TODO(bykoianko) Turn types Gate and LiftGate should be removed.
                if (HasType(m_typeLiftGate, types))
                    turns.emplace_back(i, turns::PedestrianDirection::LiftGate);
                else if (HasType(m_typeGate, types))
                    turns.emplace_back(i, turns::PedestrianDirection::Gate);
            }
        }

        // direction "arrival"
        // (index of last junction is the same as number of edges)
        turns.emplace_back(routeEdges.size(), turns::PedestrianDirection::ReachedYourDestination);
    }

    void PedestrianDirectionsEngine::SetIndexGraphLoader(IndexGraphLoader & indexGraphLoader)
    {
        m_indexGraphLoader = &indexGraphLoader;
    }

    FeaturesLoaderGuard & PedestrianDirectionsEngine::GetLoader(MwmSet::MwmId const & id)
    {
        if (!m_loader || id != m_loader->GetId())
            m_loader = make_unique<FeaturesLoaderGuard>(m_dataSource, id);
        return *m_loader;
    }

    void PedestrianDirectionsEngine::LoadPathAttributes(FeatureID const & featureId,
                                                        LoadedPathSegment & pathSegment)
    {
        if (!featureId.IsValid())
            return;

        if (FakeFeatureIds::IsGuidesFeature(featureId.m_index))
            return;




        auto ft = GetLoader(featureId.m_mwmId).GetFeatureByIndex(featureId.m_index);
        if (!ft)
            return;

        auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(*ft));
        ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
        ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

        pathSegment.m_highwayClass = highwayClass;
        pathSegment.m_isLink = ftypes::IsLinkChecker::Instance()(*ft);
        ft->GetName(StringUtf8Multilang::kDefaultCode, pathSegment.m_name);
        pathSegment.m_onRoundabout = ftypes::IsRoundAboutChecker::Instance()(*ft);

    }

    void PedestrianDirectionsEngine::LoadPathAttributesBefore(FeatureID const & featureId,
                                                              LoadedPathSegment & pathSegment)
    {
        if (!featureId.IsValid())
            return;

        if (FakeFeatureIds::IsGuidesFeature(featureId.m_index))
            return;

        auto ft = GetLoader(featureId.m_mwmId).GetFeatureByIndex(featureId.m_index);
        if (!ft)
            return;

        auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(*ft));
        ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
        ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

        pathSegment.m_highwayClass = highwayClass;
        pathSegment.m_isLink = ftypes::IsLinkChecker::Instance()(*ft);
        ft->GetName(StringUtf8Multilang::kDefaultCode, pathSegment.m_name);
        pathSegment.m_onRoundabout = ftypes::IsRoundAboutChecker::Instance()(*ft);
    }

    void PedestrianDirectionsEngine::GetEdges(IndexRoadGraph const & graph,
                                              geometry::PointWithAltitude const & currJunction,
                                              bool isCurrJunctionFinish, IRoadGraph::EdgeVector & outgoing,
                                              IRoadGraph::EdgeVector & ingoing)
    {
        // Note. If |currJunction| is a finish the outgoing edges
        // from finish are not important for turn generation.
        if (!isCurrJunctionFinish)
            graph.GetOutgoingEdges(currJunction, outgoing);

        graph.GetIngoingEdges(currJunction, ingoing);
    }

}  // namespace routing
