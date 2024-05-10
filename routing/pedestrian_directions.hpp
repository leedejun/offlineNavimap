#pragma once

#include "routing/directions_engine.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point_with_altitude.hpp"
#include "turn_candidate.hpp"
#include "loaded_path_segment.hpp"
#include "index_graph_loader.hpp"

#include <memory>
#include <vector>

namespace routing
{

    class PedestrianDirectionsEngine : public IDirectionsEngine
    {
    public:
        struct AdjacentEdges
        {
            explicit AdjacentEdges(size_t ingoingTurnsCount = 0) : m_ingoingTurnsCount(ingoingTurnsCount) {}
            bool IsAlmostEqual(AdjacentEdges const & rhs) const;

            turns::TurnCandidates m_outgoingTurns;
            size_t m_ingoingTurnsCount;
        };

        using AdjacentEdgesMap = std::map<SegmentRange, AdjacentEdges>;

        // PedestrianDirectionsEngine(std::shared_ptr<NumMwmIds> numMwmIds);
        PedestrianDirectionsEngine(DataSource const & dataSource, std::shared_ptr<NumMwmIds> numMwmIds);

        // IDirectionsEngine override:
        bool Generate(IndexRoadGraph const & graph, std::vector<geometry::PointWithAltitude> const & path,
                      base::Cancellable const & cancellable, Route::TTurns & turns,
                      Route::TStreets & streetNames,
                      std::vector<geometry::PointWithAltitude> & routeGeometry,
                      std::vector<Segment> & segments) override;
        void Clear() override {}

        void LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment);

        void LoadPathAttributesBefore(FeatureID const & featureId, LoadedPathSegment & pathSegment);

        void SetIndexGraphLoader(IndexGraphLoader & indexGraphLoader);

    private:
        FeaturesLoaderGuard & GetLoader(MwmSet::MwmId const & id);

        void GetSegmentRangeAndAdjacentEdges(IRoadGraph::EdgeVector const & outgoingEdges,
                                             Edge const & inEdge, uint32_t startSegId, uint32_t endSegId,
                                             SegmentRange & segmentRange,
                                             turns::TurnCandidates & outgoingTurns);

        /// \brief The method gathers sequence of segments according to IsJoint() method
        /// and fills |m_adjacentEdges| and |m_pathSegments|.
        void FillPathSegmentsAndAdjacentEdgesMap(IndexRoadGraph const & graph,
                                                 std::vector<geometry::PointWithAltitude> const & path,
                                                 IRoadGraph::EdgeVector const & routeEdges,
                                                 base::Cancellable const & cancellable);

        void CalculateTurns(IndexRoadGraph const & graph, std::vector<Edge> const & routeEdges,
                            Route::TTurns & turnsDir, base::Cancellable const & cancellable) const;

        void GetEdges(IndexRoadGraph const & graph, geometry::PointWithAltitude const & currJunction,
                      bool isCurrJunctionFinish, IRoadGraph::EdgeVector & outgoing,
                      IRoadGraph::EdgeVector & ingoing);

        AdjacentEdgesMap m_adjacentEdges;
        TUnpackedPathSegments m_pathSegments;
        uint32_t const m_typeSteps;
        uint32_t const m_typeLiftGate;
        uint32_t const m_typeGate;
        std::shared_ptr<NumMwmIds> const m_numMwmIds;
        DataSource const & m_dataSource;
        std::unique_ptr<FeaturesLoaderGuard> m_loader;
        IndexGraphLoader * m_indexGraphLoader;
    };

}  // namespace routing
