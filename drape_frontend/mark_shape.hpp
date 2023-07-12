#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/constants.hpp"


namespace dp
{
    class OverlayHandle;
}  // namespace dp

namespace df
{
    class MarkShape : public MapShape
    {
    public:
        MarkShape(m2::PointD const & pt, MarkViewParams const & params);

        void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                  ref_ptr<dp::TextureManager> textures) const override;

        // MapShapeType GetType() const override { return MapShapeType::OverlayType; }

    private:
        uint64_t GetOverlayPriority() const;
        drape_ptr<dp::OverlayHandle> CreateOverlayHandle(m2::PointF const & pixelSize) const;

        m2::PointD const m_pt;
        MarkViewParams const m_params;
    };
}  // namespace df