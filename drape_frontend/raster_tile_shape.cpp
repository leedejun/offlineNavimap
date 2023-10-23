#include "drape_frontend/raster_tile_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace df
{

RasterTileShape::RasterTileShape(RasterTileViewParams const & params)
:m_params(params)
{
}

void RasterTileShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const
{
    if(textures->GetCustomTexture(m_params.m_id)==nullptr)
    {
        textures->AddCustomTexture(context,
                                    m_params.m_id,
                                    m_params.m_texturePath,
                                    m_params.m_format);
    }
    
    ref_ptr<dp::Texture> tielTexture = textures->GetCustomTexture(m_params.m_id);
    m2::RectD tileRect = m_params.m_tileRect;

//    m2::RectD smallRect = m2::RectD(tileRect.minX()*0.5, tileRect.minY()*0.5, tileRect.maxX()*0.5, tileRect.maxY()*0.5);
    buffer_vector<gpu::AreaVertex, 6> vertexes;
    vertexes.resize(6);

    //0,2,1
    vertexes[0]= gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(tileRect.RightTop(), m_params.m_tileCenter, kShapeCoordScalar)),
                    m_params.m_depth), glsl::vec2(1.0f, 0.0f));
    vertexes[1] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(tileRect.RightBottom(), m_params.m_tileCenter, kShapeCoordScalar)),
                    m_params.m_depth), glsl::vec2(1.0f, 1.0f));
    vertexes[2] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(tileRect.LeftTop(), m_params.m_tileCenter, kShapeCoordScalar)),
                    m_params.m_depth), glsl::vec2(0.0f, 0.0f));
    //0,3,2
    vertexes[3] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(tileRect.RightBottom(), m_params.m_tileCenter, kShapeCoordScalar)),
                    m_params.m_depth), glsl::vec2(1.0f, 1.0f));
    vertexes[4] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(tileRect.LeftBottom(), m_params.m_tileCenter, kShapeCoordScalar)),
                    m_params.m_depth), glsl::vec2(0.0f, 1.0f));
    vertexes[5] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(tileRect.LeftTop(), m_params.m_tileCenter, kShapeCoordScalar)),
                    m_params.m_depth), glsl::vec2(0.0f, 0.0f));

    // auto state = CreateRenderState(gpu::Program::Area, DepthLayer::OverlayLayer);
    auto state = CreateRenderState(gpu::Program::Area, DepthLayer::GeometryLayer);
    
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(tielTexture);

    dp::AttributeProvider provider(1, static_cast<uint32_t>(vertexes.size()));
    provider.InitStream(0, gpu::AreaVertex::GetBindingInfo(), make_ref(vertexes.data()));
    batcher->InsertTriangleList(context, state, make_ref(&provider));


    // struct RasterTileVertex
    //     {
    //         RasterTileVertex(glsl::vec2 const & position, glsl::vec2 const & texCoord)
    //                 : m_position(position)
    //                 , m_texCoord(texCoord)
    //         {}

    //         glsl::vec2 m_position;
    //         glsl::vec2 m_texCoord;
    //     };

    //     RasterTileVertex vertexes[] =
    //             {
    //                     RasterTileVertex(glsl::ToVec2(ConvertToLocal(tileRect.LeftBottom(), m_params.m_tileCenter, kShapeCoordScalar)), glsl::vec2(0.0f, 0.0f)),
    //                     RasterTileVertex(glsl::ToVec2(ConvertToLocal(tileRect.LeftTop(), m_params.m_tileCenter, kShapeCoordScalar)), glsl::vec2(0.0f, 1.0f)),
    //                     RasterTileVertex(glsl::ToVec2(ConvertToLocal(tileRect.RightBottom(), m_params.m_tileCenter, kShapeCoordScalar)), glsl::vec2(1.0f, 0.0f)),
    //                     RasterTileVertex(glsl::ToVec2(ConvertToLocal(tileRect.RightTop(), m_params.m_tileCenter, kShapeCoordScalar)), glsl::vec2(1.0f, 1.0f))
    //             };

    //     auto state = df::CreateRenderState(gpu::Program::TexturingGui, DepthLayer::GeometryLayer);
    //     state.SetColorTexture(tielTexture);
    //     state.SetDepthTestEnabled(false);
    //     state.SetTextureIndex(0);

    //     dp::AttributeProvider provider(1, 4);
    //     dp::BindingInfo info(2);

    //     dp::BindingDecl & posDecl = info.GetBindingDecl(0);
    //     posDecl.m_attributeName = "a_position";
    //     posDecl.m_componentCount = 2;
    //     posDecl.m_componentType = gl_const::GLFloatType;
    //     posDecl.m_offset = 0;
    //     posDecl.m_stride = sizeof(RasterTileVertex);

    //     dp::BindingDecl & texDecl = info.GetBindingDecl(1);
    //     texDecl.m_attributeName = "a_colorTexCoords";
    //     texDecl.m_componentCount = 2;
    //     texDecl.m_componentType = gl_const::GLFloatType;
    //     texDecl.m_offset = sizeof(glsl::vec2);
    //     texDecl.m_stride = posDecl.m_stride;

    //     provider.InitStream(0, info, make_ref(&vertexes));
    //     batcher->InsertTriangleStrip(context, state, make_ref(&provider));
}

}  // namespace df