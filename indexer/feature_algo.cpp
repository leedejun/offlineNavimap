#include "indexer/feature_algo.hpp"
#include "indexer/feature.hpp"

#include "geometry/algorithm.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/triangle2d.hpp"

#include "base/logging.hpp"

#include <limits>

namespace feature
{
/// @returns point on a feature that is the closest to f.GetLimitRect().Center().
/// It is used by many ednities in the core of mapsme. Do not modify it's
/// logic if you really-really know what you are doing.
m2::PointD GetCenter(FeatureType & f, int scale)
{
  GeomType const type = f.GetGeomType();
  switch (type)
  {
  case GeomType::Point:
  {
    return f.GetCenter();
  }
  case GeomType::Line:
  {
    m2::CalculatePolyLineCenter doCalc;
    f.ForEachPoint(doCalc, scale);
    return doCalc.GetResult();
  }
  default:
  {
    ASSERT_EQUAL(type, GeomType::Area, ());
    m2::CalculatePointOnSurface doCalc(f.GetLimitRect(scale));
    f.ForEachTriangle(doCalc, scale);
    return doCalc.GetResult();
  }
  }
}

m2::PointD GetCenter(FeatureType & f) {
  return GetCenter(f, FeatureType::BEST_GEOMETRY); }
std::string GetAddress(FeatureType & f) {
  return f.GetHouseNumber(); }

double GetMinDistanceMeters(FeatureType & ft, m2::PointD const & pt, int scale)
{
  double res = std::numeric_limits<double>::max();
  auto updateDistanceFn = [&] (m2::PointD const & p)
  {
    double const d = mercator::DistanceOnEarth(p, pt);
    if (d < res)
      res = d;
  };

  GeomType const type = ft.GetGeomType();
  switch (type)
  {
  case GeomType::Point:
    updateDistanceFn(ft.GetCenter());
    break;

  case GeomType::Line:
  {
    ft.ParseGeometry(scale);
    size_t const count = ft.GetPointsCount();
    for (size_t i = 1; i < count; ++i)
    {
      m2::ParametrizedSegment<m2::PointD> segment(ft.GetPoint(i - 1), ft.GetPoint(i));
      updateDistanceFn(segment.ClosestPointTo(pt));
    }
    break;
  }

  default:
    ASSERT_EQUAL(type, GeomType::Area, ());
    ft.ForEachTriangle([&](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
    {
      if (res == 0.0)
        return;

      if (m2::IsPointInsideTriangle(pt, p1, p2, p3))
      {
        res = 0.0;
        return;
      }

      auto fn = [&](m2::PointD const & x1, m2::PointD const & x2) {
        m2::ParametrizedSegment<m2::PointD> const segment(x1, x2);
        updateDistanceFn(segment.ClosestPointTo(pt));
      };

      fn(p1, p2);
      fn(p2, p3);
      fn(p3, p1);

    }, scale);
    break;
  }

  return res;
}

double GetMinDistanceMeters(FeatureType & ft, m2::PointD const & pt)
{
  return GetMinDistanceMeters(ft, pt, FeatureType::BEST_GEOMETRY);
}
}  // namespace feature
