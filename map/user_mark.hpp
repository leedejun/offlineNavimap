#pragma once

#include "drape_frontend/user_marks_provider.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include <limits>
#include <memory>
#include <string>
#include <utility>

class UserMark : public df::UserPointMark
{
public:
  enum class Priority: uint16_t
  {
    Default = 0,
    RouteStart,
    RouteFinish,
    RouteIntermediateC,
    RouteIntermediateB,
    RouteIntermediateA,
    TransitStop,
    TransitGate,
    TransitTransfer,
    TransitKeyStop,
    SpeedCamera,
    RoadWarning,
    RoadWarningFirstDirty,
    RoadWarningFirstToll,
    RoadWarningFirstFerry,
      Green
  };

  enum Type : uint32_t
  {
    BOOKMARK,  // Should always be the first one //wzd 书签mark
    API,
    SEARCH,    //wzd 搜索mark
    STATIC,    //wzd 静态mark
    ROUTING,   //wzd 路线mark
    SPEED_CAM,  //wzd 速度视角mark
    ROAD_WARNING, //wzd 公路标识mark
    TRANSIT,     //wzd  通过？
    LOCAL_ADS,   //wzd 本地地址mark
    TRACK_INFO,  //wzd 轨迹信息
    TRACK_SELECTION,//wzd 选择轨迹
    GUIDE,          //wzd  手册
    GUIDE_CLUSTER,
    GUIDE_SELECTION,
    DEBUG_MARK,  // Plain "DEBUG" results in a name collision.
    COLORED,  //wzd  自定义颜色mark
    USER_MARK_TYPES_COUNT,
    USER_MARK_TYPES_COUNT_MAX = 1000,
  };

  UserMark(kml::MarkId id, m2::PointD const & ptOrg, UserMark::Type type);
  UserMark(m2::PointD const & ptOrg, UserMark::Type type);

  static Type GetMarkType(kml::MarkId id);

  Type GetMarkType() const { return GetMarkType(GetId()); }
  kml::MarkGroupId GetGroupId() const override { return GetMarkType(); }

  // df::UserPointMark overrides.
  bool IsDirty() const override { return m_isDirty; }
  void ResetChanges() const override { m_isDirty = false; }
  bool IsVisible() const override { return true; }
  m2::PointD const & GetPivot() const override;
  m2::PointD GetPixelOffset() const override { return {}; }
  dp::Anchor GetAnchor() const override { return dp::Center; }
  bool GetDepthTestEnabled() const override { return true; }
  float GetDepth() const override { return kInvalidDepth; }
  df::DepthLayer GetDepthLayer() const override { return df::DepthLayer::UserMarkLayer; }
  drape_ptr<TitlesInfo> GetTitleDecl() const override { return nullptr; }
  drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const override { return nullptr; }
  drape_ptr<BageInfo> GetBadgeInfo() const override { return nullptr; }
  drape_ptr<SymbolSizes> GetSymbolSizes() const override { return nullptr; }
  drape_ptr<SymbolOffsets> GetSymbolOffsets() const override { return nullptr; }
  uint16_t GetPriority() const override { return static_cast<uint16_t >(Priority::Default); }
  df::SpecialDisplacement GetDisplacement() const override { return df::SpecialDisplacement::UserMark; }
  uint32_t GetIndex() const override { return 0; }
  bool SymbolIsPOI() const override { return false; }
  bool HasTitlePriority() const override { return false; }
  int GetMinZoom() const override { return 1; }
  int GetMinTitleZoom() const override { return GetMinZoom(); }
  FeatureID GetFeatureID() const override { return FeatureID(); }
  bool HasCreationAnimation() const override { return false; }
  df::ColorConstant GetColorConstant() const override { return {}; }
  bool IsMarkAboveText() const override { return false; }
  float GetSymbolOpacity() const override { return 1.0f; }

  ms::LatLon GetLatLon() const;

  virtual bool IsAvailableForSearch() const { return true; }

protected:
  void SetDirty() { m_isDirty = true; }

  m2::PointD m_ptOrg;
    std::string _icon;

private:
  mutable bool m_isDirty = true;

  DISALLOW_COPY_AND_MOVE(UserMark);
};

class StaticMarkPoint : public UserMark
{
public:
  explicit StaticMarkPoint(m2::PointD const & ptOrg);

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override { return nullptr; }

  void SetPtOrg(m2::PointD const & ptOrg);
};

class MyPositionMarkPoint : public StaticMarkPoint
{
public:
  explicit MyPositionMarkPoint(m2::PointD const & ptOrg);

  void SetUserPosition(m2::PointD const & pt, bool hasPosition)
  {
    SetPtOrg(pt);
    m_hasPosition = hasPosition;
  }
  bool HasPosition() const { return m_hasPosition; }

private:
  bool m_hasPosition = false;
};

class DebugMarkPoint : public UserMark
{
public:
  explicit DebugMarkPoint(m2::PointD const & ptOrg);

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
};

class ColoredMarkPoint : public UserMark
{
public:
  explicit ColoredMarkPoint(m2::PointD const & ptOrg);

  void SetColor(dp::Color const & color);
  void SetRadius(float radius);
  bool SymbolIsPOI() const override { return true; }
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override { return nullptr; }
  drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const override;

private:
  ColoredSymbolZoomInfo m_coloredSymbols;
};

std::string DebugPrint(UserMark::Type type);
