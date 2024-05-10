#pragma once

#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "drape/render_bucket.hpp"
#include "drape/texture_manager.hpp"

#include <string>
#include <utility>
#include <vector>

namespace df
{
    struct DrapeApiRenderProperty
    {
        std::string m_id;
        m2::PointD m_center;
        std::vector<std::pair<dp::RenderState, drape_ptr<dp::RenderBucket>>> m_buckets;
    };

    class DrapeApiBuilder
    {
    public:
        void BuildLines(ref_ptr<dp::GraphicsContext> context, DrapeApi::TLines const & lines,
                        ref_ptr<dp::TextureManager> textures,
                        std::vector<drape_ptr<DrapeApiRenderProperty>> & properties);

        void BuildPolygons(ref_ptr<dp::GraphicsContext> context, DrapeApi::TPolygons const & polygons,
                           ref_ptr<dp::TextureManager> textures,
                           std::vector<drape_ptr<DrapeApiRenderProperty>> & properties);

        void BuildCustomMarks(ref_ptr<dp::GraphicsContext> context, DrapeApi::TCustomMarks const & marks,
                              ref_ptr<dp::TextureManager> textures,
                              std::vector<drape_ptr<DrapeApiRenderProperty>> & properties);
    };
}  // namespace df
