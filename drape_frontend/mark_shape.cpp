#include "drape_frontend/mark_shape.hpp"

#include "drape_frontend/color_constants.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include <utility>
#include <vector>

namespace df
{
    using SV = gpu::SolidTexturingVertex;
    using MV = gpu::MaskedTexturingVertex;

    glsl::vec2 ShiftNormal(glsl::vec2 const & n, df::MarkViewParams const & params,
                           m2::PointF const & pixelSize)
    {
        glsl::vec2 result = n + glsl::vec2(params.m_offset.x, params.m_offset.y);
        m2::PointF const halfPixelSize = pixelSize * 0.5f;

        if (params.m_anchor & dp::Top)
            result.y += halfPixelSize.y;
        else if (params.m_anchor & dp::Bottom)
            result.y -= halfPixelSize.y;

        if (params.m_anchor & dp::Left)
            result.x += halfPixelSize.x;
        else if (params.m_anchor & dp::Right)
            result.x -= halfPixelSize.x;

        return result;
    }

    template <typename TVertex>
    void Batch(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
               drape_ptr<dp::OverlayHandle> && handle, glsl::vec4 const & position,
               df::MarkViewParams const & params, ref_ptr<dp::Texture> markTexture, ref_ptr<dp::TextureManager> textures)
    {
        ASSERT(0, ("Can not be used without specialization"));
    }

    template <>
    void Batch<SV>(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                   drape_ptr<dp::OverlayHandle> && handle, glsl::vec4 const & position,
                   df::MarkViewParams const & params, ref_ptr<dp::Texture> markTexture, ref_ptr<dp::TextureManager> texMng)
    {
        m2::PointF pixelSize = params.m_markPixSize;
        m2::PointF const halfSize = pixelSize * 0.5f;
        m2::RectF const texRect = m2::RectF(0.0f, 0.0f, pixelSize.x, pixelSize.y);

        // dp::TextureManager::SymbolRegion region;
        // texMng->GetSymbolRegion("compass-image", region);
        // auto const halfSize = glsl::ToVec2(region.GetPixelSize() * 0.5f);
        // auto const texRect = region.GetTexRect();

        // std::vector<SV> vertexes;
        // vertexes.reserve(4);
        // vertexes.emplace_back(position,
        //                       ShiftNormal(glsl::vec2(-halfSize.x, halfSize.y), params, pixelSize),
        //                       glsl::vec2(texRect.minX(), texRect.maxY()));
        // vertexes.emplace_back(position,
        //                       ShiftNormal(glsl::vec2(-halfSize.x, -halfSize.y), params, pixelSize),
        //                       glsl::vec2(texRect.minX(), texRect.minY()));
        // vertexes.emplace_back(position,
        //                       ShiftNormal(glsl::vec2(halfSize.x, halfSize.y), params, pixelSize),
        //                       glsl::vec2(texRect.maxX(), texRect.maxY()));
        // vertexes.emplace_back(position,
        //                       ShiftNormal(glsl::vec2(halfSize.x, -halfSize.y), params, pixelSize),
        //                       glsl::vec2(texRect.maxX(), texRect.minY()));

        // auto state = df::CreateRenderState(gpu::Program::Texturing, DepthLayer::GeometryLayer);
        // // state.SetProgram3d(gpu::Program::TexturingBillboard);
        // state.SetDepthTestEnabled(params.m_depthTestEnabled);
        // state.SetColorTexture(markTexture);
        // state.SetTextureFilter(dp::TextureFilter::Nearest);
        // state.SetTextureIndex(0);

        // dp::AttributeProvider provider(1 /* streamCount */, vertexes.size());
        // provider.InitStream(0 /* streamIndex */, SV::GetBindingInfo(), make_ref(vertexes.data()));

        struct MarkVertex
        {
            MarkVertex(glsl::vec2 const & position, glsl::vec2 const & texCoord)
                    : m_position(position)
                    , m_texCoord(texCoord)
            {}

            glsl::vec2 m_position;
            glsl::vec2 m_texCoord;
        };

        // MarkVertex vertexes[] =
        // {
        //   MarkVertex(glsl::vec2(-halfSize.x, halfSize.y), glsl::vec2(0.0f, 1.0f)),
        //   MarkVertex(glsl::vec2(-halfSize.x, -halfSize.y), glsl::vec2(0.0f, 0.0f)),
        //   MarkVertex(glsl::vec2(halfSize.x, halfSize.y), glsl::vec2(1.0f, 1.0f)),
        //   MarkVertex(glsl::vec2(halfSize.x, -halfSize.y), glsl::vec2(1.0f, 0.0f))
        // };

        MarkVertex vertexes[] =
                {
                        MarkVertex(ShiftNormal(glsl::vec2(-halfSize.x, halfSize.y), params, pixelSize), glsl::vec2(0.0f, 1.0f)),
                        MarkVertex(ShiftNormal(glsl::vec2(-halfSize.x, -halfSize.y), params, pixelSize), glsl::vec2(0.0f, 0.0f)),
                        MarkVertex(ShiftNormal(glsl::vec2(halfSize.x, halfSize.y), params, pixelSize), glsl::vec2(1.0f, 1.0f)),
                        MarkVertex(ShiftNormal(glsl::vec2(halfSize.x, -halfSize.y), params, pixelSize), glsl::vec2(1.0f, 0.0f))
                };

        auto state = df::CreateRenderState(gpu::Program::TexturingGui, df::DepthLayer::GuiLayer);
        state.SetColorTexture(markTexture);
        state.SetDepthTestEnabled(false);
        state.SetTextureIndex(0);

        dp::AttributeProvider provider(1, 4);
        dp::BindingInfo info(2);

        dp::BindingDecl & posDecl = info.GetBindingDecl(0);
        posDecl.m_attributeName = "a_position";
        posDecl.m_componentCount = 2;
        posDecl.m_componentType = gl_const::GLFloatType;
        posDecl.m_offset = 0;
        posDecl.m_stride = sizeof(MarkVertex);

        dp::BindingDecl & texDecl = info.GetBindingDecl(1);
        texDecl.m_attributeName = "a_colorTexCoords";
        texDecl.m_componentCount = 2;
        texDecl.m_componentType = gl_const::GLFloatType;
        texDecl.m_offset = sizeof(glsl::vec2);
        texDecl.m_stride = posDecl.m_stride;

        handle->SetIsVisible(true);
        provider.InitStream(0, info, make_ref(&vertexes));
        batcher->InsertTriangleStrip(context, state, make_ref(&provider), std::move(handle));
    }

    MarkShape::MarkShape(m2::PointD const & pt, MarkViewParams const & params)
            : m_pt(pt)
            , m_params(params)
    {

    }

    uint64_t MarkShape::GetOverlayPriority() const
    {
        // Set up maximum priority
        if (m_params.m_prioritized)
            return dp::kPriorityMaskAll;

        return dp::CalculateOverlayPriority(m_params.m_minVisibleScale, m_params.m_rank, m_params.m_depth);
    }

    drape_ptr<dp::OverlayHandle> MarkShape::CreateOverlayHandle(m2::PointF const & pixelSize) const
    {
        FeatureID featureId;
        dp::OverlayID overlayId(featureId);
        drape_ptr<dp::OverlayHandle> handle = make_unique_dp<dp::SquareHandle>(overlayId, m_params.m_anchor,
                                                                               m_pt, m2::PointD(pixelSize),
                                                                               m2::PointD(m_params.m_offset),
                                                                               GetOverlayPriority(),
                                                                               true /* isBound */,
                                                                               m_params.m_id,
                                                                               m_params.m_minVisibleScale,
                                                                               true /* isBillboard */);
        handle->SetPivotZ(m_params.m_posZ);
        handle->SetOverlayRank(m_params.m_startOverlayRank);
        return handle;
    }

    void MarkShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                         ref_ptr<dp::TextureManager> textures) const
    {
        if(textures->GetCustomTexture(m_params.m_textureName)==nullptr)
        {
            textures->AddCustomTexture(context,
                                       m_params.m_textureName,
                                       m_params.m_format,
                                       m_params.m_textureData,
                                       m_params.m_textureDataSize);
        }

        glsl::vec2 const pt = glsl::ToVec2(ConvertToLocal(m_pt, m_params.m_tileCenter, kShapeCoordScalar));
        glsl::vec4 const position = glsl::vec4(pt, m_params.m_depth, -m_params.m_posZ);
        m2::PointF const pixelSize = m_params.m_markPixSize;
        Batch<SV>(context, batcher,
                  CreateOverlayHandle(pixelSize),
                  position, m_params, textures->GetCustomTexture(m_params.m_textureName), textures);
    }

}