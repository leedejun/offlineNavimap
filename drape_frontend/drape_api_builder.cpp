#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/colored_symbol_shape.hpp"
#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/mark_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/batcher.hpp"

#include "indexer/feature_decl.hpp"

#include "base/string_utils.hpp"
#include "3party/mapbox/earcut.hpp"

namespace
{
    void BuildText(ref_ptr<dp::GraphicsContext> context, std::string const & str,
                   dp::FontDecl const & font, dp::Anchor anchor, m2::PointD const & position, m2::PointD const & center,
                   ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher)
    {
        gui::StaticLabel::LabelResult result;
        gui::StaticLabel::CacheStaticText(str, "\n", anchor, font, textures, result);
        glsl::vec2 const pt = glsl::ToVec2(df::MapShape::ConvertToLocal(position, center, df::kShapeCoordScalar));
        for (gui::StaticLabel::Vertex & v : result.m_buffer)
            v.m_position = glsl::vec3(pt, 0.0f);

        dp::AttributeProvider provider(1 /* streamCount */, static_cast<uint32_t>(result.m_buffer.size()));
        provider.InitStream(0 /* streamIndex */, gui::StaticLabel::Vertex::GetBindingInfo(),
                            make_ref(result.m_buffer.data()));

        batcher.InsertListOfStrip(context, result.m_state, make_ref(&provider), dp::Batcher::VertexPerQuad);
    }
}  // namespace

namespace df
{
    void DrapeApiBuilder::BuildLines(ref_ptr<dp::GraphicsContext> context,
                                     DrapeApi::TLines const & lines, ref_ptr<dp::TextureManager> textures,
                                     std::vector<drape_ptr<DrapeApiRenderProperty>> & properties)
    {
        properties.reserve(lines.size());

        uint32_t constexpr kMaxSize = 5000;
        uint32_t constexpr kFontSize = 14;
        FeatureID fakeFeature;

        for (auto const & line : lines)
        {
            std::string id = line.first;
            DrapeApiLineData const & data = line.second;
            m2::RectD rect;
            for (auto const & p : data.m_points)
                rect.Add(p);

            dp::Batcher batcher(kMaxSize, kMaxSize);
            batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
            auto property = make_unique_dp<DrapeApiRenderProperty>();
            property->m_center = rect.Center();
            {
                dp::SessionGuard guard(context, batcher, [&property, id](dp::RenderState const & state,
                                                                         drape_ptr<dp::RenderBucket> && b)
                {
                    property->m_id = id;
                    property->m_buckets.emplace_back(state, std::move(b));
                });

                if( data.m_showLine ){
                    m2::SharedSpline spline(data.m_points);
                    LineViewParams lvp;
                    lvp.m_tileCenter = property->m_center;
                    lvp.m_depthTestEnabled = false;
                    lvp.m_minVisibleScale = 1;
                    lvp.m_cap = dp::RoundCap;
                    lvp.m_color = data.m_color;
                    lvp.m_width = data.m_width;
                    lvp.m_join = dp::RoundJoin;
                    LineShape(spline, lvp).Draw(context, make_ref(&batcher), textures);
                }

                if (data.m_showPoints)
                {
                    ColoredSymbolViewParams cvp;
                    cvp.m_tileCenter = property->m_center;
                    cvp.m_depthTestEnabled = false;
                    cvp.m_minVisibleScale = 1;
                    cvp.m_shape = ColoredSymbolViewParams::Shape::Circle;
                    cvp.m_color = data.m_color;
                    cvp.m_radiusInPixels = data.m_width * 2.0f;
                    for (m2::PointD const & pt : data.m_points)
                    {
                        ColoredSymbolShape(m2::PointD(pt), cvp, TileKey(), 0 /* textIndex */,
                                           false /* need overlay */).Draw(context, make_ref(&batcher), textures);
                    }
                }

                if (data.m_markPoints || data.m_showId)
                {
                    dp::FontDecl font(data.m_color, kFontSize);
                    size_t index = 0;
                    for (m2::PointD const & pt : data.m_points)
                    {
                        if (index > 0 && !data.m_markPoints) break;

                        std::string s;
                        if (data.m_markPoints)
                            s = strings::to_string(index) + ((data.m_showId && index == 0) ? (" (" + id + ")") : "");
                        else
                            s = id;

                        BuildText(context, s, font, dp::LeftTop, pt, property->m_center,  textures, batcher);
                        index++;
                    }
                }
            }

            if (!property->m_buckets.empty())
                properties.push_back(std::move(property));
        }
    }


    void DrapeApiBuilder::BuildPolygons(ref_ptr<dp::GraphicsContext> context, DrapeApi::TPolygons const & polygons,
                                        ref_ptr<dp::TextureManager> textures,
                                        std::vector<drape_ptr<DrapeApiRenderProperty>> & properties)
    {
        properties.reserve(polygons.size());
        uint32_t constexpr kMaxSize = 5000;
        for (auto const & polygon : polygons)
        {
            std::string id = polygon.first;
            DrapeApiPolygonData const & data = polygon.second;
            m2::RectD rect;
            for (auto const & p : data.m_points)
                rect.Add(p);

            dp::Batcher batcher(kMaxSize, kMaxSize);
            batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
            auto property = make_unique_dp<DrapeApiRenderProperty>();
            property->m_center = rect.Center();
            {
                dp::SessionGuard guard(context, batcher, [&property, id](dp::RenderState const & state,
                                                                         drape_ptr<dp::RenderBucket> && b)
                {
                    property->m_id = id;
                    property->m_buckets.emplace_back(state, std::move(b));
                });

                //build triangles of a polygon
                {
                    AreaViewParams params;
                    params.m_tileCenter = property->m_center;
                    params.m_depthTestEnabled = false;
                    params.m_minVisibleScale = 1;
                    params.m_color = data.m_color;
                    params.m_outlineColor = data.m_outlineColor;
                    params.m_baseGtoPScale = 1.0f;
                    std::vector<m2::PointD> triangleList;
                    {
                        //三角抛分:
                        //mapbox earcut
                        // // C++ 代码示例
                        // #include <earcut.hpp>
                        // 使用double类型的坐标
                        // using Coord = double;
                        // // 使用uint32_t类型的索引
                        // using N = uint32_t;
                        // // 使用std::array表示点
                        // using Point = std::array<Coord, 2>;
                        // // 创建一个包含顶点坐标的数组
                        // std::vector<std::vector<Point>> polygon;
                        // // 填充多边形数据，任何绕序都可以
                        // // 第一个折线定义了主多边形
                        // polygon.push_back({{100, 0}, {100, 100}, {0, 100}, {0, 0}});
                        // // 后面的折线定义了洞
                        // polygon.push_back({{75, 25}, {75, 75}, {25, 75}, {25, 25}});
                        // // 运行三角剖分
                        // // 返回一个包含顶点索引的数组
                        // // 比如：索引6对应于{25, 75}这个点
                        // // 三个连续的索引构成一个三角形，输出的三角形是顺时针方向的
                        // std::vector<N> indices = mapbox::earcut<N>(polygon);

                        using Coord = double;
                        // 使用uint32_t类型的索引
                        using N = uint32_t;
                        // 使用std::array表示点
                        using Point = std::array<Coord, 2>;
                        // 创建一个包含顶点坐标的数组
                        std::vector<std::vector<Point>> polygon;
                        // 填充多边形数据，任何绕序都可以
                        // 第一个折线定义了主多边形
                        std::vector<Point> poly;
                        for (auto const & p : data.m_points)
                        {
                            poly.push_back({p.x, p.y});
                        }
                        polygon.push_back(poly);
                        // 运行三角剖分
                        // 返回一个包含顶点索引的数组
                        // 三个连续的索引构成一个三角形，输出的三角形是顺时针方向的
                        std::vector<N> indices = mapbox::earcut<N>(polygon);
                        std::reverse(indices.begin(), indices.end());
                        for (auto const & index : indices)
                        {
                            triangleList.push_back(m2::PointD(poly[index].at(0), poly[index].at(1)));
                        }
                    }
                    BuildingOutline outline;
                    outline.m_generateOutline = false;
                    AreaShape(std::vector<m2::PointD>(triangleList), std::move(outline), params).Draw(context, make_ref(&batcher), textures);

                    //绘制outline
                    if(data.m_points.size())
                    {
                        std::vector<m2::PointD> outline = data.m_points;
                        outline.push_back(outline.at(0));
                        m2::SharedSpline spline(outline);
                        LineViewParams lvp;
                        lvp.m_tileCenter = property->m_center;
                        lvp.m_depthTestEnabled = false;
                        lvp.m_minVisibleScale = 1;
                        lvp.m_cap = dp::RoundCap;
                        lvp.m_color = data.m_outlineColor;
                        lvp.m_width = data.m_outlineWidth;
                        lvp.m_join = dp::RoundJoin;
                        LineShape(spline, lvp).Draw(context, make_ref(&batcher), textures);
                    }
                }
            }

            if (!property->m_buckets.empty())
                properties.push_back(std::move(property));
        }
    }

    void DrapeApiBuilder::BuildCustomMarks(ref_ptr<dp::GraphicsContext> context, DrapeApi::TCustomMarks const & marks,
                                           ref_ptr<dp::TextureManager> textures,
                                           std::vector<drape_ptr<DrapeApiRenderProperty>> & properties)
    {
        properties.reserve(marks.size());
        uint32_t constexpr kMaxSize = 5000;

        for (auto const & mark : marks)
        {
            std::string id = mark.first;
            DrapeApiCustomMarkData const & data = mark.second;
            dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
            batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
            auto property = make_unique_dp<DrapeApiRenderProperty>();
            property->m_center = data.m_center;
            {
                dp::SessionGuard guard(context, batcher, [&property, id](dp::RenderState const & state,
                                                                         drape_ptr<dp::RenderBucket> && b)
                {
                    property->m_id = id;
                    property->m_buckets.emplace_back(state, std::move(b));
                });

                MarkViewParams mvp(id);
                mvp.m_tileCenter = property->m_center;
                mvp.m_depthTestEnabled = false;
                mvp.m_minVisibleScale = 1;
                mvp.m_format = dp::TextureFormat::RGBA8;
                mvp.m_textureName = id;
                mvp.m_markPixSize = data.m_size;
                mvp.m_depth = 16432;
                mvp.m_rank = 150;
                mvp.m_anchor = dp::Bottom;
                MarkShape(data.m_center, mvp).Draw(context, make_ref(&batcher), textures);


                //text
                {
                    uint32_t constexpr kFontSize = 24;
                    dp::FontDecl font(dp::Color(255,0,0,255), kFontSize);
                    size_t index = 0;
                    std::string s;
                    s = id;
                    BuildText(context, s, font, dp::Top, property->m_center, property->m_center, textures, batcher);
                }

            }

            if (!property->m_buckets.empty())
                properties.push_back(std::move(property));
        }
    }


}  // namespace df
