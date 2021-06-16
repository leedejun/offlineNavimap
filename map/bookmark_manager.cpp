#include "map/bookmark_manager.hpp"
#include "map/api_mark_point.hpp"
#include "map/local_ads_mark.hpp"
#include "map/routing_mark.hpp"
#include "map/search_api.hpp"
#include "map/search_mark.hpp"
#include "map/user.hpp"
#include "map/user_mark.hpp"
#include "map/user_mark_id_storage.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/selection_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"

#include "coding/file_writer.hpp"
#include "coding/hex.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/sha1.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "coding/zip_creator.hpp"
#include "coding/zip_reader.hpp"

#include "geometry/rect_intersect.hpp"
#include "geometry/transformations.hpp"

#include "base/file_name_utils.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

#include "3party/Alohalytics/src/alohalytics.h"

#include "private.h"

using namespace std::placeholders;

namespace
{
size_t const kMinCommonTypesCount = 3;
double const kNearDistanceInMeters = 20 * 1000.0;
double const kMyPositionTrackSnapInMeters = 20.0;
std::string const kLargestBookmarkSymbolName = "bookmark-default-m";

class FindMarkFunctor
{
public:
  FindMarkFunctor(UserMark const ** mark, double & minD, m2::AnyRectD const & rect)
    : m_mark(mark)
    , m_minD(minD)
    , m_rect(rect)
  {
    m_globalCenter = rect.GlobalCenter();
  }

  void operator()(UserMark const * mark)
  {
    m2::PointD const & org = mark->GetPivot();
    if (m_rect.IsPointInside(org))
    {
      double minDCandidate = m_globalCenter.SquaredLength(org);
      if (minDCandidate < m_minD)
      {
        *m_mark = mark;
        m_minD = minDCandidate;
      }
    }
  }

  UserMark const ** m_mark;
  double & m_minD;
  m2::AnyRectD const & m_rect;
  m2::PointD m_globalCenter;
};

}  // namespace

using namespace std::placeholders;

BookmarkManager::BookmarkManager():
  m_changesTracker(this)
  , m_drapeChangesTracker(this)
  , m_needTeardown(false)
{
  m_userMarkLayers.reserve(UserMark::USER_MARK_TYPES_COUNT - 1);
  for (uint32_t i = 1; i < UserMark::USER_MARK_TYPES_COUNT; ++i)
    m_userMarkLayers.emplace_back(std::make_unique<UserMarkLayer>(static_cast<UserMark::Type>(i)));

  m_selectionMark = CreateUserMark<StaticMarkPoint>(m2::PointD(116.58516,43.80836));
  m_myPositionMark = CreateUserMark<MyPositionMarkPoint>(m2::PointD{});

  m_trackInfoMarkId = CreateUserMark<TrackInfoMark>(m2::PointD{})->GetId();
}

BookmarkManager::EditSession BookmarkManager::GetEditSession()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return EditSession(*this);
}

UserMark const * BookmarkManager::GetMark(kml::MarkId markId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetUserMark(markId);
}

UserMark const * BookmarkManager::GetUserMark(kml::MarkId markId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_userMarks.find(markId);
  return (it != m_userMarks.end()) ? it->second.get() : nullptr;
}

UserMark * BookmarkManager::GetUserMarkForEdit(kml::MarkId markId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_userMarks.find(markId);
  if (it != m_userMarks.end())
  {
    m_changesTracker.OnUpdateMark(markId);
    return it->second.get();
  }
  return nullptr;
}

void BookmarkManager::DeleteUserMark(kml::MarkId markId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_userMarks.find(markId);
  auto const groupId = it->second->GetGroupId();
  GetGroup(groupId)->DetachUserMark(markId);
  m_changesTracker.OnDeleteMark(markId);
  m_userMarks.erase(it);
}

Track * BookmarkManager::CreateTrack(kml::TrackData && trackData)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return AddTrack(std::make_unique<Track>(std::move(trackData), false /* interactive */));
}

Track const * BookmarkManager::GetTrack(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  return (it != m_tracks.end()) ? it->second.get() : nullptr;
}

void BookmarkManager::AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  it->second->Attach(groupId);
//  GetBmCategory(groupId)->AttachTrack(trackId);
}

void BookmarkManager::DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
//  GetBmCategory(groupId)->DetachTrack(trackId);
}

void BookmarkManager::DeleteTrack(kml::TrackId trackId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  DeleteTrackSelectionMark(trackId);
  auto it = m_tracks.find(trackId);
  auto const groupId = it->second->GetGroupId();
//  if (groupId != kml::kInvalidMarkGroupId)
//    GetBmCategory(groupId)->DetachTrack(trackId);
  m_changesTracker.OnDeleteLine(trackId);
  m_tracks.erase(it);
}

void BookmarkManager::GetDirtyGroups(kml::GroupIdSet & dirtyGroups) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & group : m_userMarkLayers)
  {
    if (!group->IsDirty())
      continue;
    auto const groupId = static_cast<kml::MarkGroupId>(group->GetType());
    dirtyGroups.insert(groupId);
  }
//  for (auto const & group : m_categories)
//  {
//    if (!group.second->IsDirty())
//      continue;
//    dirtyGroups.insert(group.first);
//  }
}

void BookmarkManager::OnEditSessionOpened()
{
  ++m_openedEditSessionsCount;
}

void BookmarkManager::OnEditSessionClosed()
{
  ASSERT_GREATER(m_openedEditSessionsCount, 0, ());
  if (--m_openedEditSessionsCount == 0)
    NotifyChanges();
}

void BookmarkManager::NotifyChanges()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_changesTracker.AcceptDirtyItems();
  if (!m_firstDrapeNotification &&
    !m_changesTracker.HasChanges() &&
    !m_drapeChangesTracker.HasChanges())
  {
    return;
  }

  m_drapeChangesTracker.AddChanges(m_changesTracker);
  m_changesTracker.ResetChanges();

  if (!m_notificationsEnabled)
    return;

  if (!m_drapeChangesTracker.HasChanges())
    return;

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
  {
    auto engine = lock.Get();
    for (auto groupId : m_drapeChangesTracker.GetUpdatedGroupIds())
    {
      auto * group = GetGroup(groupId);
      engine->ChangeVisibilityUserMarksGroup(groupId, group->IsVisible());
    }

    for (auto groupId : m_drapeChangesTracker.GetRemovedGroupIds())
      engine->ClearUserMarksGroup(groupId);

    engine->UpdateUserMarks(&m_drapeChangesTracker, m_firstDrapeNotification);
    m_firstDrapeNotification = false;

    engine->InvalidateUserMarks();
    m_drapeChangesTracker.ResetChanges();
  }
}

kml::MarkIdSet const & BookmarkManager::GetUserMarkIds(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->GetUserMarks();
}

kml::TrackIdSet const & BookmarkManager::GetTrackIds(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->GetUserLines();
}


// static
std::string BookmarkManager::GetNearMeSortedBlockName()
{
  return platform::GetLocalizedString("near_me_sorttype");
}

// static
//bool BookmarkManager::IsGuide(kml::AccessRules accessRules)
//{
//  return accessRules == kml::AccessRules::Public || accessRules == kml::AccessRules::Paid ||
//         accessRules == kml::AccessRules::P2P;
//}
BookmarkManager::TrackSelectionInfo BookmarkManager::FindNearestTrack(
    m2::RectD const & touchRect, TracksFilter const & tracksFilter) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  TrackSelectionInfo selectionInfo;

//  auto minSquaredDist = std::numeric_limits<double>::max();
//  for (auto const & pair : m_categories)
//  {
//    auto const & category = *pair.second;
//    if (!category.IsVisible())
//      continue;
//
//    for (auto trackId : category.GetUserLines())
//    {
//      auto const track = GetTrack(trackId);
//      if (!track->IsInteractive() || (tracksFilter && !tracksFilter(track)))
//        continue;
//
//      auto const trackRect = track->GetLimitRect();
//
//      if (!trackRect.IsIntersect(touchRect))
//        continue;
//
//      auto const & pointsWithAlt = track->GetPointsWithAltitudes();
//      for (size_t i = 0; i + 1 < pointsWithAlt.size(); ++i)
//      {
//        auto pt1 = pointsWithAlt[i].GetPoint();
//        auto pt2 = pointsWithAlt[i + 1].GetPoint();
//        if (!m2::Intersect(touchRect, pt1, pt2))
//          continue;
//
//        m2::ParametrizedSegment<m2::PointD> seg(pt1, pt2);
//        auto const closestPoint = seg.ClosestPointTo(touchRect.Center());
//        auto const squaredDist = closestPoint.SquaredLength(touchRect.Center());
//        if (squaredDist >= minSquaredDist)
//          continue;
//
//        minSquaredDist = squaredDist;
//        selectionInfo.m_trackId = trackId;
//        selectionInfo.m_trackPoint = closestPoint;
//
//        auto const segDistInMeters = mercator::DistanceOnEarth(pointsWithAlt[i].GetPoint(),
//                                                               closestPoint);
//        selectionInfo.m_distanceInMeters = segDistInMeters + track->GetLengthMeters(i);
//      }
//    }
//  }

  return selectionInfo;
}

BookmarkManager::TrackSelectionInfo BookmarkManager::GetTrackSelectionInfo(kml::TrackId const & trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const markId = GetTrackSelectionMarkId(trackId);
  if (markId == kml::kInvalidMarkId)
    return {};

  auto const mark = GetMark<TrackSelectionMark>(markId);
  return TrackSelectionInfo(trackId, mark->GetPivot(), mark->GetDistance());
}

kml::MarkId BookmarkManager::GetTrackSelectionMarkId(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK_NOT_EQUAL(trackId, kml::kInvalidTrackId, ());

  for (auto markId : GetUserMarkIds(UserMark::Type::TRACK_SELECTION))
  {
    auto const * mark = GetMark<TrackSelectionMark>(markId);
    if (mark->GetTrackId() == trackId)
      return markId;
  }
  return kml::kInvalidMarkId;
}

int BookmarkManager::GetTrackSelectionMarkMinZoom(kml::TrackId trackId) const
{
  auto track = GetTrack(trackId);
  CHECK(track != nullptr, ());

  auto const zoom = std::min(df::GetDrawTileScale(track->GetLimitRect()), 14);
  return zoom;
}

void BookmarkManager::SetTrackSelectionMark(kml::TrackId trackId, m2::PointD const & pt,
                                            double distance)
{
  auto const markId = GetTrackSelectionMarkId(trackId);

  TrackSelectionMark * trackSelectionMark = nullptr;
  if (markId == kml::kInvalidMarkId)
  {
    trackSelectionMark = CreateUserMark<TrackSelectionMark>(pt);
    trackSelectionMark->SetTrackId(trackId);

    if (m_drapeEngine)
      trackSelectionMark->SetMinVisibleZoom(GetTrackSelectionMarkMinZoom(trackId));
  }
  else
  {
    trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
    trackSelectionMark->SetPosition(pt);
  }
  trackSelectionMark->SetDistance(distance);

  auto const isVisible = IsVisible(GetTrack(trackId)->GetGroupId());
  trackSelectionMark->SetIsVisible(isVisible);
}

void BookmarkManager::DeleteTrackSelectionMark(kml::TrackId trackId)
{
  if (trackId == m_selectedTrackId)
    m_selectedTrackId = kml::kInvalidTrackId;

  auto const markId = GetTrackSelectionMarkId(trackId);
  if (markId != kml::kInvalidMarkId)
    DeleteUserMark(markId);

  ResetTrackInfoMark(trackId);
}

void BookmarkManager::SetTrackInfoMark(kml::TrackId trackId, m2::PointD const & pt)
{
  auto trackInfoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
  trackInfoMark->SetPosition(pt);
  auto const isVisible = IsVisible(GetTrack(trackId)->GetGroupId());
  trackInfoMark->SetIsVisible(isVisible);
  trackInfoMark->SetTrackId(trackId);
}

void BookmarkManager::ResetTrackInfoMark(kml::TrackId trackId)
{
  auto trackInfoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
  if (trackInfoMark->GetTrackId() == trackId)
  {
    trackInfoMark->SetPosition(m2::PointD::Zero());
    trackInfoMark->SetIsVisible(false);
    trackInfoMark->SetTrackId(kml::kInvalidTrackId);
  }
}

void BookmarkManager::SetTrackSelectionInfo(TrackSelectionInfo const & trackSelectionInfo,
                                            bool notifyListeners)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK_NOT_EQUAL(trackSelectionInfo.m_trackId, kml::kInvalidTrackId, ());

  auto es = GetEditSession();
  auto const markId = GetTrackSelectionMarkId(trackSelectionInfo.m_trackId);
  CHECK_NOT_EQUAL(markId, kml::kInvalidMarkId, ());

  auto trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
  trackSelectionMark->SetPosition(trackSelectionInfo.m_trackPoint);
  trackSelectionMark->SetDistance(trackSelectionInfo.m_distanceInMeters);

//  if (notifyListeners && m_elevationActivePointChanged != nullptr)
//    m_elevationActivePointChanged();
}

void BookmarkManager::SetDefaultTrackSelection(kml::TrackId trackId, bool showInfoSign)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto track = GetTrack(trackId);
  CHECK(track != nullptr, ());
  CHECK(track->IsInteractive(), ());

  auto const & points = track->GetPointsWithAltitudes();
  auto const pt = points[points.size() / 2].GetPoint();
  auto const distance = track->GetLengthMeters(points.size() / 2);

  auto es = GetEditSession();
  if (showInfoSign)
    SetTrackInfoMark(trackId, pt);
  SetTrackSelectionMark(trackId, pt, distance);
}

void BookmarkManager::OnTrackSelected(kml::TrackId trackId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto es = GetEditSession();
  ResetTrackInfoMark(trackId);

  auto const markId = GetTrackSelectionMarkId(trackId);
  CHECK_NOT_EQUAL(markId, kml::kInvalidMarkId, ());

  auto * trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
  trackSelectionMark->SetIsVisible(false);

  m_selectedTrackId = trackId;
}

void BookmarkManager::OnTrackDeselected()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_selectedTrackId == kml::kInvalidTrackId)
    return;

  auto const markId = GetTrackSelectionMarkId(m_selectedTrackId);
  CHECK_NOT_EQUAL(markId, kml::kInvalidMarkId, ());

  auto es = GetEditSession();
  auto * trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
  auto const isVisible = IsVisible(GetTrack(m_selectedTrackId)->GetGroupId());
  trackSelectionMark->SetIsVisible(isVisible);

  m_selectedTrackId = kml::kInvalidTrackId;
}



void BookmarkManager::ClearGroup(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto * group = GetGroup(groupId);
  for (auto markId : group->GetUserMarks())
  {
      m_userMarks.erase(markId);
    m_changesTracker.OnDeleteMark(markId);
  }
  for (auto trackId : group->GetUserLines())
  {
    DeleteTrackSelectionMark(trackId);
    m_changesTracker.OnDeleteLine(trackId);
    m_tracks.erase(trackId);
  }
  group->Clear();
}

UserMark const * BookmarkManager::FindMarkInRect(kml::MarkGroupId groupId, m2::AnyRectD const & rect,
                                                 bool findOnlyVisible, double & d) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const * group = GetGroup(groupId);

  UserMark const * resMark = nullptr;
  if (group->IsVisible())
  {
    FindMarkFunctor f(&resMark, d, rect);
    for (auto markId : group->GetUserMarks())
    {
      auto const * mark = GetMark(markId);
      if (findOnlyVisible && !mark->IsVisible())
        continue;

      if (mark->IsAvailableForSearch() && rect.IsPointInside(mark->GetPivot()))
        f(mark);
    }
  }
  return resMark;
}

void BookmarkManager::SetIsVisible(kml::MarkGroupId groupId, bool visible)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  GetGroup(groupId)->SetIsVisible(visible);
  UpdateTrackMarksVisibility(groupId);
}

bool BookmarkManager::IsVisible(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->IsVisible();
}

void BookmarkManager::UpdateTrackMarksMinZoom()
{
  auto const marksIds = GetUserMarkIds(UserMark::TRACK_SELECTION);
  for (auto markId : marksIds)
  {
    auto mark = GetMarkForEdit<TrackSelectionMark>(markId);
    mark->SetMinVisibleZoom(GetTrackSelectionMarkMinZoom(mark->GetTrackId()));
  }
}

void BookmarkManager::UpdateTrackMarksVisibility(kml::MarkGroupId groupId)
{
  auto const isVisible = IsVisible(groupId);
  auto const tracksIds = GetTrackIds(groupId);
  auto infoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
  for (auto trackId : tracksIds)
  {
    auto markId = GetTrackSelectionMarkId(trackId);
    if (markId == kml::kInvalidMarkId)
      continue;
    if (infoMark->GetTrackId() == trackId && infoMark->IsVisible())
      infoMark->SetIsVisible(isVisible);
    auto mark = GetMarkForEdit<TrackSelectionMark>(markId);
    mark->SetIsVisible(isVisible);
  }
}

void BookmarkManager::RequestSymbolSizes()
{
  std::vector<std::string> symbols;
  symbols.push_back(kLargestBookmarkSymbolName);
  symbols.push_back(TrackSelectionMark::GetInitialSymbolName());

  m_drapeEngine.SafeCall(
      &df::DrapeEngine::RequestSymbolsSize, symbols,
      [this](std::map<std::string, m2::PointF> && sizes) {
        GetPlatform().RunTask(Platform::Thread::Gui, [this, sizes = std::move(sizes)]() mutable {
          auto es = GetEditSession();
          auto infoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
          auto const & sz = sizes.at(TrackSelectionMark::GetInitialSymbolName());
          infoMark->SetOffset(m2::PointF(0.0, -sz.y / 2));
          m_maxBookmarkSymbolSize = sizes.at(kLargestBookmarkSymbolName);
          m_symbolSizesAcquired = true;
          if (m_onSymbolSizesAcquiredFn)
            m_onSymbolSizesAcquiredFn();
        });
      });
}

void BookmarkManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_drapeEngine.Set(engine);
  m_firstDrapeNotification = true;

  auto es = GetEditSession();
  UpdateTrackMarksMinZoom();
  RequestSymbolSizes();
}

void BookmarkManager::UpdateViewport(ScreenBase const & screen)
{
  m_viewport = screen;
}

void BookmarkManager::Teardown()
{
  m_needTeardown = true;
}

Track * BookmarkManager::AddTrack(std::unique_ptr<Track> && track)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto * t = track.get();
  auto const trackId = t->GetId();
  CHECK_EQUAL(m_tracks.count(trackId), 0, ());
  m_tracks.emplace(trackId, std::move(track));
  m_changesTracker.OnAddLine(trackId);
  return t;
}

namespace
{
class BestUserMarkFinder
{
public:
  explicit BestUserMarkFinder(BookmarkManager::TTouchRectHolder const & rectHolder,
                              BookmarkManager::TFindOnlyVisibleChecker const & findOnlyVisible,
                              BookmarkManager const * manager)
    : m_rectHolder(rectHolder)
    , m_findOnlyVisible(findOnlyVisible)
    , m_d(std::numeric_limits<double>::max())
    , m_mark(nullptr)
    , m_manager(manager)
  {}

  bool operator()(kml::MarkGroupId groupId)
  {
//    auto const groupType = BookmarkManager::GetGroupType(groupId);
//    if (auto const * p = m_manager->FindMarkInRect(groupId, m_rectHolder(groupType), m_findOnlyVisible(groupType), m_d))
//    {
//      m_mark = p;
//      return true;
//    }
    return false;
  }

  UserMark const * GetFoundMark() const { return m_mark; }

private:
  BookmarkManager::TTouchRectHolder const m_rectHolder;
  BookmarkManager::TFindOnlyVisibleChecker const m_findOnlyVisible;
  double m_d;
  UserMark const * m_mark;
  BookmarkManager const * m_manager;
};
}  // namespace

UserMark const * BookmarkManager::FindNearestUserMark(m2::AnyRectD const & rect) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return FindNearestUserMark([&rect](UserMark::Type) { return rect; }, [](UserMark::Type) { return false; });
}

UserMark const * BookmarkManager::FindNearestUserMark(TTouchRectHolder const & holder,
                                                      TFindOnlyVisibleChecker const & findOnlyVisible) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Among the marks inside the rect (if any) finder stores the closest one to its center.
  BestUserMarkFinder finder(holder, findOnlyVisible, this);

  // Look for the closest mark among GUIDE and GUIDE_CLUSTER marks.
  finder(UserMark::Type::GUIDE);
  finder(UserMark::Type::GUIDE_CLUSTER);

  if (finder.GetFoundMark() != nullptr)
    return finder.GetFoundMark();

  // For each type X in the condition, ordered by priority:
  //  - look for the closest mark among the marks of the same type X.
  //  - if the mark has been found, stop looking for a closer one in the other types.
  if (finder(UserMark::Type::ROUTING) ||
      finder(UserMark::Type::ROAD_WARNING) ||
      finder(UserMark::Type::SEARCH) ||
      finder(UserMark::Type::API))
  {
    return finder.GetFoundMark();
  }

  // Look for the closest bookmark.
//  for (auto const & pair : m_categories)
//    finder(pair.first);

  if (finder.GetFoundMark() != nullptr)
    return finder.GetFoundMark();

  // Look for the closest TRACK_INFO or TRACK_SELECTION mark.
  finder(UserMark::Type::TRACK_INFO);
  finder(UserMark::Type::TRACK_SELECTION);

  return finder.GetFoundMark();
}

UserMarkLayer const * BookmarkManager::GetGroup(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::USER_MARK_TYPES_COUNT)
  {
    CHECK_GREATER(groupId, 0, ());
    return m_userMarkLayers[static_cast<size_t>(groupId - 1)].get();
  }

//  ASSERT(m_categories.find(groupId) != m_categories.end(), ());
//  return m_categories.at(groupId).get();
  return nullptr;
}

UserMarkLayer * BookmarkManager::GetGroup(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::USER_MARK_TYPES_COUNT)
  {
    CHECK_GREATER(groupId, 0, ());
    return m_userMarkLayers[static_cast<size_t>(groupId - 1)].get();
  }

//  auto const it = m_categories.find(groupId);
//  return it != m_categories.end() ? it->second.get() : nullptr;
  return nullptr;
}


bool BookmarkManager::IsEditableTrack(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const * track = GetTrack(trackId);
  CHECK(track != nullptr, ());
//  if (track->GetGroupId() != kml::kInvalidMarkGroupId)
//    return IsEditableCategory(track->GetGroupId());
  return true;
}

kml::GroupIdSet BookmarkManager::MarksChangesTracker::GetAllGroupIds() const
{
  auto const & groupIds = m_bmManager->GetBmGroupsIdList();
  kml::GroupIdSet resultingSet(groupIds.begin(), groupIds.end());
  for (uint32_t i = 1; i < UserMark::USER_MARK_TYPES_COUNT; ++i)
    resultingSet.insert(static_cast<kml::MarkGroupId>(i));
  return resultingSet;
}

bool BookmarkManager::MarksChangesTracker::IsGroupVisible(kml::MarkGroupId groupId) const
{
  return m_bmManager->IsVisible(groupId);
}

kml::MarkIdSet const & BookmarkManager::MarksChangesTracker::GetGroupPointIds(kml::MarkGroupId groupId) const
{
  return m_bmManager->GetUserMarkIds(groupId);
}

kml::TrackIdSet const & BookmarkManager::MarksChangesTracker::GetGroupLineIds(kml::MarkGroupId groupId) const
{
  return m_bmManager->GetTrackIds(groupId);
}

df::UserPointMark const * BookmarkManager::MarksChangesTracker::GetUserPointMark(kml::MarkId markId) const
{
  return m_bmManager->GetMark(markId);
}

df::UserLineMark const * BookmarkManager::MarksChangesTracker::GetUserLineMark(kml::TrackId lineId) const
{
  return m_bmManager->GetTrack(lineId);
}

void BookmarkManager::MarksChangesTracker::OnAddMark(kml::MarkId markId)
{
  m_createdMarks.insert(markId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteMark(kml::MarkId markId)
{
  auto const it = m_createdMarks.find(markId);
  if (it != m_createdMarks.end())
  {
    m_createdMarks.erase(it);
  }
  else
  {
    m_updatedMarks.erase(markId);
    m_removedMarks.insert(markId);
  }
}

void BookmarkManager::MarksChangesTracker::OnUpdateMark(kml::MarkId markId)
{
  if (m_createdMarks.find(markId) == m_createdMarks.end())
    m_updatedMarks.insert(markId);
}

void BookmarkManager::MarksChangesTracker::OnAddLine(kml::TrackId lineId)
{
  m_createdLines.insert(lineId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteLine(kml::TrackId lineId)
{
  auto const it = m_createdLines.find(lineId);
  if (it != m_createdLines.end())
    m_createdLines.erase(it);
  else
    m_removedLines.insert(lineId);
}

void BookmarkManager::MarksChangesTracker::OnAddGroup(kml::MarkGroupId groupId)
{
  m_createdGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteGroup(kml::MarkGroupId groupId)
{
  m_updatedGroups.erase(groupId);
  m_becameVisibleGroups.erase(groupId);
  m_becameInvisibleGroups.erase(groupId);

  auto const it = m_createdGroups.find(groupId);
  if (it != m_createdGroups.end())
    m_createdGroups.erase(it);
  else
    m_removedGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnUpdateGroup(kml::MarkGroupId groupId)
{
  m_updatedGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnBecomeVisibleGroup(kml::MarkGroupId groupId)
{
  auto const it = m_becameInvisibleGroups.find(groupId);
  if (it != m_becameInvisibleGroups.end())
    m_becameInvisibleGroups.erase(it);
  else
    m_becameVisibleGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnBecomeInvisibleGroup(kml::MarkGroupId groupId)
{
  auto const it = m_becameVisibleGroups.find(groupId);
  if (it != m_becameVisibleGroups.end())
    m_becameVisibleGroups.erase(it);
  else
    m_becameInvisibleGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::AcceptDirtyItems()
{
  CHECK(m_updatedGroups.empty(), ());
  m_bmManager->GetDirtyGroups(m_updatedGroups);
  for (auto groupId : m_updatedGroups)
  {
    auto group = m_bmManager->GetGroup(groupId);
    if (group->IsVisibilityChanged())
    {
      if (group->IsVisible())
        m_becameVisibleGroups.insert(groupId);
      else
        m_becameInvisibleGroups.insert(groupId);
    }
    group->ResetChanges();
  }

  kml::MarkIdSet dirtyMarks;
  for (auto const markId : m_updatedMarks)
  {
    auto const mark = m_bmManager->GetMark(markId);
    if (mark->IsDirty())
    {
      dirtyMarks.insert(markId);
      m_updatedGroups.insert(mark->GetGroupId());
      mark->ResetChanges();
    }
  }
  m_updatedMarks.swap(dirtyMarks);

  for (auto const markId : m_createdMarks)
  {
    auto const mark = m_bmManager->GetMark(markId);
    CHECK(mark->IsDirty(), ());
    mark->ResetChanges();
  }

  for (auto const lineId : m_createdLines)
  {
    auto const line = m_bmManager->GetTrack(lineId);
    CHECK(line->IsDirty(), ());
    line->ResetChanges();
  }
}

bool BookmarkManager::MarksChangesTracker::HasChanges() const
{
  return !m_updatedGroups.empty() || !m_removedGroups.empty();
}

bool BookmarkManager::MarksChangesTracker::HasCategoriesChanges() const
{
  return false;// HasBookmarkCategories(m_createdGroups) || HasBookmarkCategories(m_removedGroups);
}

void BookmarkManager::MarksChangesTracker::ResetChanges()
{
  m_createdGroups.clear();
  m_removedGroups.clear();

  m_updatedGroups.clear();
  m_becameVisibleGroups.clear();
  m_becameInvisibleGroups.clear();

  m_createdMarks.clear();
  m_removedMarks.clear();
  m_updatedMarks.clear();

  m_createdLines.clear();
  m_removedLines.clear();
}

void BookmarkManager::MarksChangesTracker::AddChanges(MarksChangesTracker const & changes)
{
  if (!HasChanges())
  {
    *this = changes;
    return;
  }

  for (auto const groupId : changes.m_createdGroups)
    OnAddGroup(groupId);

  for (auto const groupId : changes.m_updatedGroups)
    OnUpdateGroup(groupId);

  for (auto const groupId : changes.m_becameVisibleGroups)
    OnBecomeVisibleGroup(groupId);

  for (auto const groupId : changes.m_becameInvisibleGroups)
    OnBecomeInvisibleGroup(groupId);

  for (auto const groupId : changes.m_removedGroups)
    OnDeleteGroup(groupId);

  for (auto const markId : changes.m_createdMarks)
    OnAddMark(markId);

  for (auto const markId : changes.m_updatedMarks)
    OnUpdateMark(markId);

  for (auto const markId : changes.m_removedMarks)
    OnDeleteMark(markId);

  for (auto const lineId : changes.m_createdLines)
    OnAddLine(lineId);

  for (auto const lineId : changes.m_removedLines)
    OnDeleteLine(lineId);
}

// static
std::string BookmarkManager::GenerateUniqueFileName(const std::string & path, std::string name, std::string const & kmlExt)
{
  // check if file name already contains .kml extension
  size_t const extPos = name.rfind(kmlExt);
  if (extPos != std::string::npos)
  {
    // remove extension
    ASSERT_GREATER_OR_EQUAL(name.size(), kmlExt.size(), ());
    size_t const expectedPos = name.size() - kmlExt.size();
    if (extPos == expectedPos)
      name.resize(expectedPos);
  }

  size_t counter = 1;
  std::string suffix;
  while (Platform::IsFileExistsByFullPath(base::JoinPath(path, name + suffix + kmlExt)))
    suffix = strings::to_string(counter++);
  return base::JoinPath(path, name + suffix + kmlExt);
}

BookmarkManager::EditSession::EditSession(BookmarkManager & manager)
  : m_bmManager(manager)
{
  m_bmManager.OnEditSessionOpened();
}

BookmarkManager::EditSession::~EditSession()
{
  m_bmManager.OnEditSessionClosed();
}

Track * BookmarkManager::EditSession::CreateTrack(kml::TrackData && trackData)
{
  return m_bmManager.CreateTrack(std::move(trackData));
}

void BookmarkManager::EditSession::DeleteUserMark(kml::MarkId markId)
{
  m_bmManager.DeleteUserMark(markId);
}

void BookmarkManager::EditSession::DeleteTrack(kml::TrackId trackId)
{
  CHECK(m_bmManager.IsEditableTrack(trackId), ());
  m_bmManager.DeleteTrack(trackId);
}

void BookmarkManager::EditSession::ClearGroup(kml::MarkGroupId groupId)
{
  m_bmManager.ClearGroup(groupId);
}

void BookmarkManager::EditSession::SetIsVisible(kml::MarkGroupId groupId, bool visible)
{
  m_bmManager.SetIsVisible(groupId, visible);
}

void BookmarkManager::EditSession::AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
//  CHECK(m_bmManager.IsEditableCategory(groupId), ());
  m_bmManager.AttachTrack(trackId, groupId);
}

void BookmarkManager::EditSession::DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
//  CHECK(m_bmManager.IsEditableCategory(groupId), ());
  m_bmManager.DetachTrack(trackId, groupId);
}
void BookmarkManager::EditSession::NotifyChanges()
{
  m_bmManager.NotifyChanges();
}
