#pragma once

//#include "map/bookmark.hpp"
//#include "map/bookmark_catalog.hpp"
//#include "map/bookmark_helpers.hpp"
//#include "map/cloud.hpp"
#include "map/elevation_info.hpp"
#include "map/track.hpp"
#include "map/track_mark.hpp"
#include "map/user_mark_layer.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "platform/safe_callback.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/macros.hpp"
#include "base/strings_bundle.hpp"
#include "base/thread_checker.hpp"
#include "base/visitor.hpp"

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class BookmarkManager final
{
  using UserMarkLayers = std::vector<std::unique_ptr<UserMarkLayer>>;
//  using CategoriesCollection = std::map<kml::MarkGroupId, std::unique_ptr<BookmarkCategory>>;

  using MarksCollection = std::map<kml::MarkId, std::unique_ptr<UserMark>>;
  using TracksCollection = std::map<kml::TrackId, std::unique_ptr<Track>>;

public:
  using OnSymbolSizesAcquiredCallback = std::function<void()>;

  class EditSession
  {
  public:
    explicit EditSession(BookmarkManager & bmManager);
    ~EditSession();

    template <typename UserMarkT>
    UserMarkT * CreateUserMark(m2::PointD const & ptOrg)
    {
      return m_bmManager.CreateUserMark<UserMarkT>(ptOrg);
    }

    Track * CreateTrack(kml::TrackData && trackData);

    template <typename UserMarkT>
    UserMarkT * GetMarkForEdit(kml::MarkId markId)
    {
      return m_bmManager.GetMarkForEdit<UserMarkT>(markId);
    }

    template <typename UserMarkT, typename F>
    void DeleteUserMarks(UserMark::Type type, F && deletePredicate)
    {
      return m_bmManager.DeleteUserMarks<UserMarkT>(type, std::move(deletePredicate));
    }

    void DeleteUserMark(kml::MarkId markId);
    void DeleteTrack(kml::TrackId trackId);

    void ClearGroup(kml::MarkGroupId groupId);

    void SetIsVisible(kml::MarkGroupId groupId, bool visible);

    void AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);
    void DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);
    void NotifyChanges();

  private:
    BookmarkManager & m_bmManager;
  };

  BookmarkManager();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  EditSession GetEditSession();

  void UpdateViewport(ScreenBase const & screen);
  void Teardown();

  template <typename UserMarkT>
  UserMarkT const * GetMark(kml::MarkId markId) const
  {
    auto * mark = GetUserMark(markId);
    ASSERT(dynamic_cast<UserMarkT const *>(mark) != nullptr, ());
    return static_cast<UserMarkT const *>(mark);
  }

  UserMark const * GetUserMark(kml::MarkId markId) const;
  Track const * GetTrack(kml::TrackId trackId) const;

  kml::MarkIdSet const & GetUserMarkIds(kml::MarkGroupId groupId) const;
  kml::TrackIdSet const & GetTrackIds(kml::MarkGroupId groupId) const;

  bool IsVisible(kml::MarkGroupId groupId) const;

  kml::GroupIdCollection const & GetBmGroupsIdList() const { return m_bmGroupsIdList; }

  using TTouchRectHolder = std::function<m2::AnyRectD(UserMark::Type)>;
  using TFindOnlyVisibleChecker = std::function<bool(UserMark::Type)>;
  UserMark const * FindNearestUserMark(TTouchRectHolder const & holder,
                                       TFindOnlyVisibleChecker const & findOnlyVisible) const;
  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect) const;
  UserMark const * FindMarkInRect(kml::MarkGroupId groupId, m2::AnyRectD const & rect, bool findOnlyVisible,
                                  double & d) const;

  StaticMarkPoint & SelectionMark() { return *m_selectionMark; }
  StaticMarkPoint const & SelectionMark() const { return *m_selectionMark; }

  MyPositionMarkPoint & MyPositionMark() { return *m_myPositionMark; }
  MyPositionMarkPoint const & MyPositionMark() const { return *m_myPositionMark; }

  enum class CategoryFilterType
  {
    Private = 0,
    Public,
    All
  };

  bool IsEditableTrack(kml::TrackId trackId) const;

  void FilterInvalidTracks(kml::TrackIdCollection & tracks) const;

  static std::string GenerateUniqueFileName(std::string const & path, std::string name, std::string const & fileExt);
  static bool IsMigrated();
  static std::string GetTracksSortedBlockName();
  static std::string GetOthersSortedBlockName();
  static std::string GetNearMeSortedBlockName();
  enum class SortedByTimeBlockType : uint32_t
  {
    WeekAgo,
    MonthAgo,
    MoreThanMonthAgo,
    MoreThanYearAgo,
    Others
  };
  static std::string GetSortedByTimeBlockName(SortedByTimeBlockType blockType);
  ElevationInfo MakeElevationInfo(kml::TrackId trackId) const;

  void UpdateElevationMyPosition(kml::TrackId const & trackId);
  // Returns distance from the start of the track to my position in meters.
  // Returns negative value if my position is not on the track.
  double GetElevationMyPosition(kml::TrackId const & trackId) const;

  struct TrackSelectionInfo
  {
    TrackSelectionInfo() = default;
    TrackSelectionInfo(kml::TrackId trackId, m2::PointD const & trackPoint, double distanceInMeters)
      : m_trackId(trackId)
      , m_trackPoint(trackPoint)
      , m_distanceInMeters(distanceInMeters)
    {}

    kml::TrackId m_trackId = kml::kInvalidTrackId;
    m2::PointD m_trackPoint = m2::PointD::Zero();
    double m_distanceInMeters = 0.0;
  };

  using TracksFilter = std::function<bool(Track const * track)>;
  TrackSelectionInfo FindNearestTrack(m2::RectD const & touchRect,
                                      TracksFilter const & tracksFilter = nullptr) const;
  TrackSelectionInfo GetTrackSelectionInfo(kml::TrackId const & trackId) const;

  void SetTrackSelectionInfo(TrackSelectionInfo const & trackSelectionInfo, bool notifyListeners);
  void SetDefaultTrackSelection(kml::TrackId trackId, bool showInfoSign);
  void OnTrackSelected(kml::TrackId trackId);
  void OnTrackDeselected();

private:
  class MarksChangesTracker : public df::UserMarksProvider
  {
  public:
    explicit MarksChangesTracker(BookmarkManager * bmManager)
      : m_bmManager(bmManager)
    {
      CHECK(m_bmManager != nullptr, ());
    }

    void OnAddMark(kml::MarkId markId);
    void OnDeleteMark(kml::MarkId markId);
    void OnUpdateMark(kml::MarkId markId);

    void OnAddLine(kml::TrackId lineId);
    void OnDeleteLine(kml::TrackId lineId);

    void OnAddGroup(kml::MarkGroupId groupId);
    void OnDeleteGroup(kml::MarkGroupId groupId);

    void AcceptDirtyItems();
    bool HasChanges() const;
    bool HasCategoriesChanges() const;
    void ResetChanges();
    void AddChanges(MarksChangesTracker const & changes);

    using GroupMarkIdSet = std::map<kml::MarkGroupId, kml::MarkIdSet>;

    // UserMarksProvider
    kml::GroupIdSet GetAllGroupIds() const override;
    kml::GroupIdSet const & GetUpdatedGroupIds() const override { return m_updatedGroups; }
    kml::GroupIdSet const & GetRemovedGroupIds() const override { return m_removedGroups; }
    kml::MarkIdSet const & GetCreatedMarkIds() const override { return m_createdMarks; }
    kml::MarkIdSet const & GetRemovedMarkIds() const override { return m_removedMarks; }
    kml::MarkIdSet const & GetUpdatedMarkIds() const override { return m_updatedMarks; }
    kml::TrackIdSet const & GetCreatedLineIds() const override { return m_createdLines; }
    kml::TrackIdSet const & GetRemovedLineIds() const override { return m_removedLines; }
    kml::GroupIdSet const & GetBecameVisibleGroupIds() const override { return m_becameVisibleGroups; }
    kml::GroupIdSet const & GetBecameInvisibleGroupIds() const override { return m_becameInvisibleGroups; }
    bool IsGroupVisible(kml::MarkGroupId groupId) const override;
    kml::MarkIdSet const & GetGroupPointIds(kml::MarkGroupId groupId) const override;
    kml::TrackIdSet const & GetGroupLineIds(kml::MarkGroupId groupId) const override;
    df::UserPointMark const * GetUserPointMark(kml::MarkId markId) const override;
    df::UserLineMark const * GetUserLineMark(kml::TrackId lineId) const override;

  private:
    void OnUpdateGroup(kml::MarkGroupId groupId);
    void OnBecomeVisibleGroup(kml::MarkGroupId groupId);
    void OnBecomeInvisibleGroup(kml::MarkGroupId groupId);

    BookmarkManager * m_bmManager;

    kml::MarkIdSet m_createdMarks;
    kml::MarkIdSet m_removedMarks;
    kml::MarkIdSet m_updatedMarks;

    kml::TrackIdSet m_createdLines;
    kml::TrackIdSet m_removedLines;

    kml::GroupIdSet m_createdGroups;
    kml::GroupIdSet m_removedGroups;

    kml::GroupIdSet m_updatedGroups;
    kml::GroupIdSet m_becameVisibleGroups;
    kml::GroupIdSet m_becameInvisibleGroups;
  };

  template <typename UserMarkT>
  UserMarkT * CreateUserMark(m2::PointD const & ptOrg)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    auto mark = std::make_unique<UserMarkT>(ptOrg);
    auto * m = mark.get();
    auto const markId = m->GetId();
    auto const groupId = static_cast<kml::MarkGroupId>(m->GetMarkType());
    CHECK_EQUAL(m_userMarks.count(markId), 0, ());
    ASSERT_GREATER(groupId, 0, ());
    ASSERT_LESS(groupId - 1, m_userMarkLayers.size(), ());
    m_userMarks.emplace(markId, std::move(mark));
    m_changesTracker.OnAddMark(markId);
    m_userMarkLayers[static_cast<size_t>(groupId - 1)]->AttachUserMark(markId);
    return m;
  }

  template <typename UserMarkT>
  UserMarkT * GetMarkForEdit(kml::MarkId markId)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    auto * mark = GetUserMarkForEdit(markId);
    ASSERT(dynamic_cast<UserMarkT *>(mark) != nullptr, ());
    return static_cast<UserMarkT *>(mark);
  }

  template <typename UserMarkT, typename F>
  void DeleteUserMarks(UserMark::Type type, F && deletePredicate)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    std::list<kml::MarkId> marksToDelete;
    for (auto markId : GetUserMarkIds(type))
    {
      if (deletePredicate(GetMark<UserMarkT>(markId)))
        marksToDelete.push_back(markId);
    }
    // Delete after iterating to avoid iterators invalidation issues.
    for (auto markId : marksToDelete)
      DeleteUserMark(markId);
  }

  UserMark * GetUserMarkForEdit(kml::MarkId markId);
  void DeleteUserMark(kml::MarkId markId);

  Track * CreateTrack(kml::TrackData && trackData);

  void AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);
  void DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);
  void DeleteTrack(kml::TrackId trackId);

  void ClearGroup(kml::MarkGroupId groupId);
  void SetIsVisible(kml::MarkGroupId groupId, bool visible);
  UserMark const * GetMark(kml::MarkId markId) const;

  UserMarkLayer const * GetGroup(kml::MarkGroupId groupId) const;
  UserMarkLayer * GetGroup(kml::MarkGroupId groupId);
  Track * AddTrack(std::unique_ptr<Track> && track);

  void OnEditSessionOpened();
  void OnEditSessionClosed();
  void NotifyChanges();

  void GetDirtyGroups(kml::GroupIdSet & dirtyGroups) const;

  struct SortTrackData
  {
    SortTrackData() = default;
    SortTrackData(kml::TrackData const & trackData)
      : m_id(trackData.m_id)
      , m_timestamp(trackData.m_timestamp)
    {}

    kml::TrackId m_id;
    kml::Timestamp m_timestamp;
  };


  kml::MarkId GetTrackSelectionMarkId(kml::TrackId trackId) const;
  int GetTrackSelectionMarkMinZoom(kml::TrackId trackId) const;
  void SetTrackSelectionMark(kml::TrackId trackId, m2::PointD const & pt, double distance);
  void DeleteTrackSelectionMark(kml::TrackId trackId);
  void SetTrackInfoMark(kml::TrackId trackId, m2::PointD const & pt);
  void ResetTrackInfoMark(kml::TrackId trackId);

  void UpdateTrackMarksMinZoom();
  void UpdateTrackMarksVisibility(kml::MarkGroupId groupId);
  void RequestSymbolSizes();

  ThreadChecker m_threadChecker;

  MarksChangesTracker m_changesTracker;
  MarksChangesTracker m_drapeChangesTracker;
  df::DrapeEngineSafePtr m_drapeEngine;

  m2::PointD m_lastElevationMyPosition = m2::PointD::Zero();

  OnSymbolSizesAcquiredCallback m_onSymbolSizesAcquiredFn;
  bool m_symbolSizesAcquired = false;

  std::atomic<bool> m_needTeardown;
  size_t m_openedEditSessionsCount = 0;
  bool m_loadBookmarksFinished = false;
  bool m_firstDrapeNotification = false;
  bool m_restoreApplying = false;
  bool m_migrationInProgress = false;
  bool m_conversionInProgress = false;
  bool m_notificationsEnabled = true;

  ScreenBase m_viewport;

  kml::GroupIdCollection m_bmGroupsIdList;

  std::string m_lastCategoryUrl;
  kml::MarkGroupId m_lastEditedGroupId = kml::kInvalidMarkGroupId;
  kml::PredefinedColor m_lastColor = kml::PredefinedColor::Red;
  UserMarkLayers m_userMarkLayers;

  MarksCollection m_userMarks;
  TracksCollection m_tracks;

  StaticMarkPoint * m_selectionMark = nullptr;
  MyPositionMarkPoint * m_myPositionMark = nullptr;

  kml::MarkId m_trackInfoMarkId = kml::kInvalidMarkId;
  kml::TrackId m_selectedTrackId = kml::kInvalidTrackId;
  m2::PointF m_maxBookmarkSymbolSize;

  bool m_asyncLoadingInProgress = false;
  bool m_testModeEnabled = false;

  DISALLOW_COPY_AND_MOVE(BookmarkManager);
};
