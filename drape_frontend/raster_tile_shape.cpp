#include "drape_frontend/raster_tile_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"


#include "indexer/map_style_reader.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include "platform/platform.hpp"

namespace df
{

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
// 读取二进制文件到字节缓冲区
std::vector<char> read_binary_file(const std::string& file_name) {
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_name << std::endl;
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Failed to read file: " << file_name << std::endl;
        return {};
    }
    return buffer;
}

// 解析.terrain文件格式的dem数据
void parse_terrain_file(const std::string& file_name)
{
    // 读取文件到缓冲区
    std::vector<char> buffer = read_binary_file(file_name);
    if (buffer.empty()) {
        return;
    }
    // 定义一些常量
    const int HEADER_SIZE = 40; // 头部大小，包含10个double类型的值
    const int VERTEX_COUNT_OFFSET = 0; // 顶点数量的偏移量
    const int U_OFFSET = 4; // u坐标的偏移量
    const int V_OFFSET = 6; // v坐标的偏移量
    const int HEIGHT_OFFSET = 8; // 高程的偏移量
    const int CHILD_TILE_MASK_OFFSET = 10; // 子瓦片掩码的偏移量
    const int WATER_MASK_OFFSET = 11; // 水面掩码的偏移量
    const int MAX_SHORT = 32767; // unsigned short的最大值
    // 读取头部信息，包含10个double类型的值，每个8个字节
    double* header = reinterpret_cast<double*>(buffer.data());
    // 读取顶点数量，为unsigned int类型，4个字节
    unsigned int vertex_count = *reinterpret_cast<unsigned int*>(buffer.data() + HEADER_SIZE + VERTEX_COUNT_OFFSET);
    // 读取u坐标，为unsigned short类型的数组，每个2个字节
    unsigned short* u = reinterpret_cast<unsigned short*>(buffer.data() + HEADER_SIZE + U_OFFSET);
    // 读取v坐标，为unsigned short类型的数组，每个2个字节
    unsigned short* v = reinterpret_cast<unsigned short*>(buffer.data() + HEADER_SIZE + V_OFFSET);
    // 读取高程，为unsigned short类型的数组，每个2个字节
    unsigned short* height = reinterpret_cast<unsigned short*>(buffer.data() + HEADER_SIZE + HEIGHT_OFFSET);
    // 读取子瓦片掩码，为unsigned char类型，1个字节
    unsigned char child_tile_mask = *reinterpret_cast<unsigned char*>(buffer.data() + HEADER_SIZE + CHILD_TILE_MASK_OFFSET);
    // 读取水面掩码，为unsigned char类型的数组，剩余的字节
    unsigned char* water_mask = reinterpret_cast<unsigned char*>(buffer.data() + HEADER_SIZE + WATER_MASK_OFFSET);
    // 输出一些信息
    std::cout << "File name: " << file_name << std::endl;
    std::cout << "Vertex count: " << vertex_count << std::endl;
    std::cout << "Child tile mask: " << static_cast<int>(child_tile_mask) << std::endl;
    // 遍历每个顶点，计算真实的坐标和高程
    for (int i = 0; i < vertex_count; i++) {
        // u和v坐标都是0-1之间的比例值，乘以MAX_SHORT还原
        double u_coord = static_cast<double>(u[i]) / MAX_SHORT;
        double v_coord = static_cast<double>(v[i]) / MAX_SHORT;
        // 高程是最小高度和最大高度之间的比例值，乘以MAX_SHORT还原，再加上最小高度
        double h = header[9] + (header[8] - header[9]) * static_cast<double>(height[i]) / MAX_SHORT;
        // 输出每个顶点的坐标和高程
        std::cout << "Vertex " << i << ": (" << u_coord << ", " << v_coord << ", " << h << ")" << std::endl;
    }
}


RasterTileShape::RasterTileShape(RasterTileViewParams const & params)
:m_params(params)
{
}

void RasterTileShape::BuildMesh(m2::RectD tileRect, const int divideNum, buffer_vector<gpu::AreaVertex, 24576>& vertexes) const
{
    if (divideNum<=0 || !tileRect.IsValid())
    {
        return;
    }
    
    if ((divideNum*divideNum*6)>24576)
    {
        vertexes.resize(divideNum*divideNum);
    }
    float divideUV = 1.0f/float(divideNum);
    float divideX = tileRect.SizeX()/float(divideNum);
    float divideY = tileRect.SizeY()/float(divideNum);

    // 用一个双重循环遍历每个小格子
    for (int i = 0; i < divideNum; i++) {
        for (int j = 0; j < divideNum; j++) {
            int k = (i * divideNum + j) * 6;
            // 计算每个小格子的左下角顶点的坐标
            double x = double(i) * divideX + tileRect.minX();
            double y = double(j) * divideY + tileRect.minY();
            // 将每个小格子的四个顶点坐标存入数组
            // vertexes[k] = {{x, y}, {x, y + divideY}, {x + divideX, y}, {x + divideX, y + divideY}};

            // 计算每个小格子的左下角uv的坐标
            double u = double(i) * divideUV;
            double v = 1.0 - double(j) * divideUV;
            // 将每个小格子的四个uv坐标存入数组
            // uvs[k] = {{u, v}, {u, v - divideUV}, {u + divideUV, v}, {u + divideUV, v - divideUV}};

            m2::Point<double> rightTop = m2::Point<double>(x + divideX, y + divideY);
            m2::Point<double> rightBottom = m2::Point<double>(x + divideX, y);
            m2::Point<double> leftTop = m2::Point<double>(x, y + divideY);
            m2::Point<double> leftBottom = m2::Point<double>(x,y);


            glsl::vec3 test = glsl::vec3(glsl::ToVec2(ConvertToLocal(rightTop, m_params.m_tileCenter, kShapeCoordScalar)),
                                         m_params.m_depth);

             //0,2,1
             vertexes[k]= gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(rightTop, m_params.m_tileCenter, kShapeCoordScalar)),
                             m_params.m_depth), glsl::vec2(u + divideUV, v - divideUV));
             vertexes[k+1] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(rightBottom, m_params.m_tileCenter, kShapeCoordScalar)),
                             m_params.m_depth), glsl::vec2(u + divideUV, v));
             vertexes[k+2] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(leftTop, m_params.m_tileCenter, kShapeCoordScalar)),
                             m_params.m_depth), glsl::vec2(u, v - divideUV));
             //0,3,2
             vertexes[k+3] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(rightBottom, m_params.m_tileCenter, kShapeCoordScalar)),
                             m_params.m_depth), glsl::vec2(u + divideUV, v));
             vertexes[k+4] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(leftBottom, m_params.m_tileCenter, kShapeCoordScalar)),
                             m_params.m_depth), glsl::vec2(u, v));
             vertexes[k+5] = gpu::AreaVertex(glsl::vec3(glsl::ToVec2(ConvertToLocal(leftTop, m_params.m_tileCenter, kShapeCoordScalar)),
                             m_params.m_depth), glsl::vec2(u, v - divideUV));
        }
    }

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

//     //add heightmapTex
//     std::string heightmapPath = "/storage/emulated/0/MapsWithMe/terrain_mapbox";
// //    std::string heightmapPath = "/storage/emulated/0/MapsWithMe/terrain";
//     heightmapPath = heightmapPath + "/" + m_params.strZ + "/" + m_params.strX + "/"+ m_params.strY + ".png";
//     std::string heightmapTexId = heightmapPath;
//     if (!Platform::IsFileExistsByFullPath(heightmapPath))
//     {
//       heightmapPath = "/storage/emulated/0/MapsWithMe/heightmap.png";
//       heightmapTexId = "heightmapTex";
//         LOG(LINFO, (std::string("File NOT Exists:")+heightmapPath));
// //        return;
//     }
//     else
//     {
//         std::cout<<"FileExists:"<<heightmapPath<<std::endl;
//         LOG(LINFO, (std::string("FileExists:")+heightmapPath));
//     }

// //     heightmapPath = "/storage/emulated/0/MapsWithMe/heightmap.png";
// //     heightmapTexId = "heightmapTex";

//     if(textures->GetCustomTexture(heightmapTexId)==nullptr)
//     {
//         textures->AddCustomTexture(context,
//                                     heightmapTexId,
//                                     heightmapPath,
//                                     m_params.m_format);
//     }
    
//     ref_ptr<dp::Texture> heightmapTex = textures->GetCustomTexture(heightmapTexId);

    m2::RectD tileRect = m_params.m_tileRect;
    // buffer_vector<gpu::AreaVertex, 24576> vertexes;
    // vertexes.resize(24576);
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

    // BuildMesh(tileRect, 64, vertexes);

    auto state = CreateRenderState(gpu::Program::Area, DepthLayer::Geometry3dLayer);
    // auto state = CreateRenderState(gpu::Program::Dem, DepthLayer::Geometry3dLayer);
    
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(tielTexture);
    // state.SetTexture("u_heightmapTex", heightmapTex);

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


//    //test
//    static int testflag=0;
//    if(testflag==0)
//    {
//    testflag=1;
//    // std::string terrainfilePath = "/storage/emulated/0/MapsWithMe/10_1685_741.terrain";
//    std::string terrainfilePath = "/storage/emulated/0/MapsWithMe/6_105_46.terrain";
//    parse_terrain_file(terrainfilePath);
//    }
}

}  // namespace df