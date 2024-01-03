#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape/utils/vertex_decl.hpp"

namespace df
{

class RasterTileShape : public MapShape
{
public:
  RasterTileShape(RasterTileViewParams const & params);
  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;

private:
  void BuildMesh(m2::RectD tileRect, const int divideNum, buffer_vector<gpu::AreaVertex, 24576>& vertexes) const;
  

  RasterTileViewParams m_params;
};

}  // namespace df