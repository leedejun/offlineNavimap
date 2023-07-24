//#include "map/framework_ft.hpp"
#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"
#include "com/mapswithme/platform/GuiThread.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "platform/network_policy.hpp"
#include "drape_frontend/user_marks_provider.hpp"
#include "platform/measurement_utils.hpp"
#include "FtMap.hpp"
//#include "platform/Platform.hpp"
#include "map/gps_tracker.hpp"
#include "map/user_mark_id_storage.hpp"
#include "map/fd/map_engine.hpp"

#include <string>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <tools/build/src/engine/strings.h>
#include <com/mapswithme/platform/Platform.hpp>

#include "3party/boost/boost/geometry.hpp"
#include "3party/boost/boost/geometry/geometries/point_xy.hpp"
#include "3party/boost/boost/geometry/geometries/polygon.hpp"

namespace {
    void OnRenderingInitializationFinished(std::shared_ptr<jobject> const &listener) {
        JNIEnv *env = jni::GetEnv();
        env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener.get(),
                                                        "onRenderingInitializationFinished",
                                                        "()V"));
    }
}

namespace bg = boost::geometry;
typedef bg::model::d2::point_xy<double> Point;
typedef bg::model::linestring<Point> Linestring;
typedef bg::model::multi_linestring<Linestring> MultiLinestring;
typedef bg::model::polygon<Point> Polygon;

#define cmd CommandHelper::getIns()
#define json JsonHelper::getIns()


template<class TT>
inline uint32_t toIntFromHexString(const TT &t) {

    uint32_t x;
    std::stringstream ss;
    ss << std::hex << t;
    ss >> x;
    return x;

}


void Stringsplit(const std::string &str, const std::string &splits, std::vector<std::string> &res) {
    if (str == "") return;
    //在字符串末尾也加入分隔符，方便截取最后一段
    std::string strs = str + splits;
    size_t pos = strs.find(splits);
    int step = splits.size();

    // 若找不到内容则字符串搜索函数返回 npos
    while (pos != strs.npos) {
        std::string temp = strs.substr(0, pos);
        res.push_back(temp);
        //去掉已分割的字符串,在剩下的字符串中进行分割
        strs = strs.substr(pos + step, strs.size());
        pos = strs.find(splits);
    }
}

inline dp::Color stringToDpColor(std::string colorString) {
    if (colorString[0] == '#') {
        colorString.erase(0, 1);
    }

    std::string stringR = colorString.substr(0, 2);
    std::string stringG = colorString.substr(2, 2);
    std::string stringB = colorString.substr(4, 2);
    std::string stringA = colorString.substr(6, 2);

    uint32_t r = toIntFromHexString(stringR);
    uint32_t g = toIntFromHexString(stringG);
    uint32_t b = toIntFromHexString(stringB);
    uint32_t a = 255;//toIntFromHexString(stringA);
    if (stringA.length() > 0) {
        a = toIntFromHexString(stringA);
    }
    return dp::Color(r, g, b, a);
}

using UserMarkLayers = std::vector<std::unique_ptr<UserMarkLayer>>;
using MarksCollection = std::map<kml::MarkId, std::unique_ptr<UserMark>>;
using TracksCollection = std::map<kml::TrackId, std::unique_ptr<df::UserLineMark>>;

::Framework *frm() { return g_framework->NativeFramework(); }


/****************************************************************************/
//LocationStateHelper
/****************************************************************************/
static void LocationStateModeChanged(location::EMyPositionMode mode,
                                     std::shared_ptr<jobject> const &listener) {
    g_framework->OnMyPositionModeChanged(mode);

    JNIEnv *env = jni::GetEnv();
    env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener.get(),
                                                    "onMyPositionModeChanged", "(I)V"),
                        static_cast<jint>(mode));
}

static void LocationPendingTimeout(std::shared_ptr<jobject> const &listener) {
    JNIEnv *env = jni::GetEnv();
    env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener.get(),
                                                    "onLocationPendingTimeout", "()V"));
}


/****************************************************************************/
//MyUserMark
/****************************************************************************/

struct MyUserMarkData {
    std::string m_title;
    std::string m_subTitle;
    RouteMarkType m_pointType = RouteMarkType::Start;
    size_t m_intermediateIndex = 0;
    bool m_isVisible = true;
    bool m_isMyPosition = false;
    bool m_isPassed = false;
    m2::PointD m_position;

};

float const kMyUserMarkSecondaryTextSize = 10.0f;
float const kMyUserMarkSecondaryOffsetY = 2.0f;
float const kMyUserMarkPrimaryTextSize = 10.5f;
static std::string const kMyUserMarkPrimaryText = "MyUserMarkPrimaryText";
static std::string const kMyUserMarkPrimaryTextOutline = "MyUserMarkPrimaryTextOutline";
static std::string const kMyUserMarkSecondaryText = "MyUserMarkSecondaryText";
static std::string const kMyUserMarkSecondaryTextOutline = "MyUserMarkSecondaryTextOutline";

class MyUserMark : public UserMark {

public:
    MyUserMark(m2::PointD const &ptOrg) : UserMark(ptOrg, UserMark::Type::SEARCH) {
        m_titleDecl.m_anchor = dp::Top;
        m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(kMyUserMarkPrimaryText);
        m_titleDecl.m_primaryTextFont.m_outlineColor = df::GetColorConstant(
                kMyUserMarkPrimaryTextOutline);
        m_titleDecl.m_primaryTextFont.m_size = kMyUserMarkPrimaryTextSize;
        m_titleDecl.m_secondaryTextFont.m_color = df::GetColorConstant(kMyUserMarkSecondaryText);
        m_titleDecl.m_secondaryTextFont.m_outlineColor = df::GetColorConstant(
                kMyUserMarkSecondaryTextOutline);
        m_titleDecl.m_secondaryTextFont.m_size = kMyUserMarkSecondaryTextSize;
        m_titleDecl.m_secondaryOffset = m2::PointF(0, kMyUserMarkSecondaryOffsetY);

        m_markData.m_position = ptOrg;
    }

    void setPos(m2::PointD p) {
        m_ptOrg = p;
    }

    void setIcon(std::string icon) {
        _icon = icon;
    }

    MyUserMarkData const &GetMarkData() const { return m_markData; }

    void SetMarkData(MyUserMarkData &&data) {
        SetDirty();
        m_markData = std::move(data);
        m_titleDecl.m_primaryText = m_markData.m_title;
        if (!m_titleDecl.m_primaryText.empty()) {
            m_titleDecl.m_secondaryText = m_markData.m_subTitle;
            m_titleDecl.m_secondaryOptional = true;
        } else
            m_titleDecl.m_secondaryText.clear();
    }

    drape_ptr<df::UserPointMark::TitlesInfo> GetTitleDecl() const {
        if (m_followingMode)
            return nullptr;

        auto titles = make_unique_dp<TitlesInfo>();
        titles->push_back(m_titleDecl);
        return titles;
    }

    virtual drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const;

private:
    MyUserMarkData m_markData;
    dp::TitleDecl m_titleDecl;
    bool m_followingMode = false;
};

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> MyUserMark::GetSymbolNames() const {
    auto symbol = make_unique_dp<df::UserPointMark::SymbolNameZoomInfo>();

    symbol->emplace(1, _icon);
    return symbol;
}

/****************************************************************************/
//MyUserLineMark
/****************************************************************************/
class MyUserLineMark : public df::UserLineMark {
    bool _isDirty;
    dp::Color _color;
    std::vector<m2::PointD> _points;
public:
    MyUserLineMark() : df::UserLineMark(UserMarkIdStorage::Instance().GetNextTrackId()) {

    }

    void addPoint(double x, double y) {
        _points.push_back(m2::PointD(x, y));
    }

    virtual bool IsDirty() const {
        return _isDirty;
    }

    virtual void ResetChanges() const {

    }

    virtual kml::MarkGroupId GetGroupId() const {
        return 0;
    }

    virtual int GetMinZoom() const {
        return 1;
    }

    virtual df::DepthLayer GetDepthLayer() const {
        return df::DepthLayer::UserMarkLayer;
    }

    virtual size_t GetLayerCount() const {
        return 1;
    }

    virtual dp::Color GetColor(size_t layerIndex) const {
        return _color;
    }


    virtual float GetWidth(size_t layerIndex) const {
        return 10.0f;
    }

    virtual float GetDepth(size_t layerIndex) const {
        return 1.0f;
    }

    virtual std::vector<m2::PointD> GetPoints() const {
        return _points;
    }
};


class UserMarkerManager : public df::UserMarksProvider {
    kml::MarkIdSet m_createdMarks;
    kml::MarkIdSet m_removedMarks;
    kml::MarkIdSet m_updatedMarks;
    kml::MarkIdSet m_groupMarks;

    kml::TrackIdSet m_createdLines;
    kml::TrackIdSet m_removedLines;
    kml::TrackIdSet m_groupLines;

    kml::GroupIdSet m_createdGroups;
    kml::GroupIdSet m_removedGroups;
    kml::GroupIdSet m_allGroups;

    kml::GroupIdSet m_updatedGroups;
    kml::GroupIdSet m_becameVisibleGroups;
    kml::GroupIdSet m_becameInvisibleGroups;
    UserMarkLayers m_userMarkLayers;
    MarksCollection m_userMarks;
    TracksCollection m_uerLines;
    ThreadChecker m_threadChecker;
    bool m_bIsFirstUpdate = true;
public:
    UserMarkerManager() {
        m_allGroups.insert(0);
    }

    virtual kml::GroupIdSet const &GetUpdatedGroupIds() const {
        return m_allGroups;
    }

    virtual kml::GroupIdSet const &GetRemovedGroupIds() const {
        return m_updatedGroups;
    }

    virtual kml::GroupIdSet GetAllGroupIds() const {

        return m_allGroups;
    }

    virtual kml::GroupIdSet const &GetBecameVisibleGroupIds() const {
        return m_allGroups;
    }

    virtual kml::GroupIdSet const &GetBecameInvisibleGroupIds() const {
        return m_allGroups;
    }

    virtual bool IsGroupVisible(kml::MarkGroupId groupId) const {
        return true;
    }

    virtual kml::MarkIdSet const &GetGroupPointIds(kml::MarkGroupId groupId) const {

        return m_groupMarks;
    }

    virtual kml::TrackIdSet const &GetGroupLineIds(kml::MarkGroupId groupId) const {
        return m_groupLines;
    }

    virtual kml::MarkIdSet const &GetCreatedMarkIds() const {
        return m_createdMarks;
    }

    virtual kml::MarkIdSet const &GetRemovedMarkIds() const {
        return m_removedMarks;
    }

    virtual kml::MarkIdSet const &GetUpdatedMarkIds() const {
        return m_updatedMarks;
    }

    virtual kml::TrackIdSet const &GetCreatedLineIds() const {
        return m_createdLines;
    }

    virtual kml::TrackIdSet const &GetRemovedLineIds() const {
        return m_removedLines;
    }

    /// Never store UserPointMark reference.
    virtual df::UserPointMark const *GetUserPointMark(kml::MarkId markId) const {
        auto it = m_userMarks.find(markId);
        return (it != m_userMarks.end()) ? it->second.get() : nullptr;
    }

    /// Never store UserLineMark reference.
    virtual df::UserLineMark const *GetUserLineMark(kml::TrackId lineId) const {
        auto it = m_uerLines.find(lineId);
        return (it != m_uerLines.end()) ? it->second.get() : nullptr;
    }

    template<typename UserMarkT>
    UserMarkT *CreateUserMark(m2::PointD const &ptOrg) {
        CHECK_THREAD_CHECKER(m_threadChecker, ());
        auto mark = std::make_unique<UserMarkT>(ptOrg);

        auto *m = mark.get();
        auto const markId = m->GetId();
        auto const groupId = static_cast<kml::MarkGroupId>(m->GetMarkType());
        m_userMarks.emplace(markId, std::move(mark));
        m_createdMarks.insert(markId);
        m_groupMarks.insert(markId);
        notifyChanges();
        return m;
    }

    MyUserLineMark *CreateUserLineMark() {
        CHECK_THREAD_CHECKER(m_threadChecker, ());
        auto mark = std::make_unique<MyUserLineMark>();
        auto *m = mark.get();
        auto const markId = m->GetId();
        m_uerLines.emplace(m->GetId(), std::move(mark));
        m_createdLines.insert(markId);
        m_groupLines.insert(markId);
//        notifyChanges();
        return m;
    }

    void RemoveUserPointMark(long id) {
        m_removedMarks.insert(id);
        m_groupMarks.erase(id);
        this->notifyChanges();
    }

    void RemoveUserPointMark(long long id) {
        m_removedMarks.insert(id);
        m_groupMarks.erase(id);
        this->notifyChanges();
    }

    void UpdateUserPointMark(long id) {
        m_updatedMarks.insert(id);
        m_groupMarks.insert(id);
        this->notifyChanges();
    }

    void UpdateUserPointMark(long long id) {
        m_updatedMarks.insert(id);
        m_groupMarks.insert(id);
        this->notifyChanges();
    }

    void notifyChanges() {
        g_framework->NativeFramework()->GetDrapeEngine()->ChangeVisibilityUserMarksGroup(0, true);
        g_framework->NativeFramework()->GetDrapeEngine()->UpdateUserMarks(this, m_bIsFirstUpdate);
        m_bIsFirstUpdate = false;
        g_framework->NativeFramework()->GetDrapeEngine()->InvalidateUserMarks();
        m_createdMarks.clear();// .erase(m_createdMarks.begin(),m_createdMarks.size());
        m_updatedMarks.clear();
        m_removedMarks.clear();
//        m_groupMarks.clear();
    }
};

static UserMarkerManager userMarks;


#define ENGINE fd::MapEngine::get
extern "C"
{
void CallRoutingListener(std::shared_ptr<jobject> listener, int errorCode,
                         storage::CountriesSet const &absentMaps) {
    JNIEnv *env = jni::GetEnv();
    jmethodID const method = jni::GetMethodID(env, *listener, "onRoutingEvent",
                                              "(I[Ljava/lang/String;)V");
    ASSERT(method, ());

    env->CallVoidMethod(*listener, method, errorCode, jni::TScopedLocalObjectArrayRef(env,
                                                                                      jni::ToJavaStringArray(
                                                                                              env,
                                                                                              absentMaps)).get());
}
void CallRouteProgressListener(std::shared_ptr<jobject> listener, float progress) {
    JNIEnv *env = jni::GetEnv();
    jmethodID const methodId = jni::GetMethodID(env, *listener, "onRouteBuildingProgress", "(F)V");
    env->CallVoidMethod(*listener, methodId, progress);
}
void CallRouteRecommendationListener(std::shared_ptr<jobject> listener,
                                     RoutingManager::Recommendation recommendation) {
    JNIEnv *env = jni::GetEnv();
    jmethodID const methodId = jni::GetMethodID(env, *listener, "onRecommend", "(I)V");
    env->CallVoidMethod(*listener, methodId, static_cast<int>(recommendation));
}
void CallSetRoutingLoadPointsListener(std::shared_ptr<jobject> listener, bool success) {
    JNIEnv *env = jni::GetEnv();
    jmethodID const methodId = jni::GetMethodID(env, *listener, "onRoutePointsLoaded", "(Z)V");
    env->CallVoidMethod(*listener, methodId, static_cast<jboolean>(success));
}
JNIEXPORT jobject
JNICALL
Java_com_ftmap_maps_FTMap_nativeReq(JNIEnv *env, jclass clazz, jobject msg) {
    std::string cmdName = cmd.getStr(msg, "_command");
    if (cmdName == "initPlatform") {
        jobject instance = cmd.getObj(msg, "thisInstance");
        jobject context = cmd.getObj(msg, "context");
        jstring apkPath = cmd.getNativeStr(msg, "apkPath");
        jstring storagePath = cmd.getNativeStr(msg, "storagePath");
        jstring filesPath = cmd.getNativeStr(msg, "filesPath");
        jstring tempPath = cmd.getNativeStr(msg, "tempPath");
        jstring obbPath = cmd.getNativeStr(msg, "obbPath");
        jstring flavor = cmd.getNativeStr(msg, "flavor");
        jstring buildType = cmd.getNativeStr(msg, "buildType");
        bool isTablet = cmd.getBool(msg, "isTablet");
        android::Platform::Instance().Initialize(env, instance, context, apkPath, storagePath, filesPath,
                                                 tempPath, obbPath, flavor, buildType, isTablet);

        if (!g_framework) {
            g_framework = std::make_unique<android::Framework>();
            // 初始化飞度引擎
            ENGINE(g_framework->NativeFramework());
        }


//        g_framework->SetupWidget(static_cast<gui::EWidget>(gui::WIDGET_RULER), 30, 1942,
//                                 static_cast<dp::Anchor>(dp::Anchor::LeftBottom));
        g_framework->SetupWidget(static_cast<gui::EWidget>(gui::WIDGET_COMPASS), 670, 1180,
                                 static_cast<dp::Anchor>(dp::Anchor::Center));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(gui::WIDGET_COPYRIGHT), 150, 1942,
//                                 static_cast<dp::Anchor>(dp::Anchor::LeftBottom));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(gui::WIDGET_SCALE_FPS_LABEL), 60, 1942,
//                                 static_cast<dp::Anchor>(dp::Anchor::LeftTop));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(gui::WIDGET_WATERMARK), 50, 1942,
//                                 static_cast<dp::Anchor>(dp::Anchor::Left));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(1), 30, 1942,
//                                 static_cast<dp::Anchor>(9));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(2), 978, 1942,
//                                 static_cast<dp::Anchor>(0));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(4), 30, 1942,
//                                 static_cast<dp::Anchor>(9));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(8), 0, 100, static_cast<dp::Anchor>(5));
//        g_framework->SetupWidget(static_cast<gui::EWidget>(10), 1050, 1942,
//                                 static_cast<dp::Anchor>(10));
        cmd.set(msg, "result", true);
    } else if (cmdName == "processTask") {
        long taskPointer = cmd.getLong(msg, "taskPointer");
        android::GuiThread::ProcessTask(taskPointer);
    } else if (cmdName == "createEngine") {
        jobject surface = cmd.getObj(msg, "surface");
        int density = cmd.getInt(msg, "density");
        bool firstLaunch = cmd.getBool(msg, "firstLaunch");
        bool isLaunchByDeepLink = cmd.getBool(msg, "isLaunchByDeepLink");
        int appVersionCode = cmd.getInt(msg, "appVersionCode");
        bool ok = g_framework->CreateDrapeEngine(env, surface, density, firstLaunch,
                                                 isLaunchByDeepLink,
                                                 appVersionCode);

        cmd.set(msg, "result", ok);
    } else if (cmdName == "onTouch") {
        int actionType = cmd.getInt(msg, "actionType");
        int id1 = cmd.getInt(msg, "id1");
        float x1 = cmd.getFloat(msg, "x1");
        float y1 = cmd.getFloat(msg, "y1");
        int id2 = cmd.getInt(msg, "id2");
        float x2 = cmd.getFloat(msg, "x2");
        float y2 = cmd.getFloat(msg, "y2");
        int maskedPointer = cmd.getInt(msg, "maskedPointer");
        g_framework->Touch(actionType,
                           android::Framework::Finger(id1, x1, y1),
                           android::Framework::Finger(id2, x2, y2), maskedPointer);

    } else if (cmdName == "attachSurface") {
        jobject surface = cmd.getObj(msg, "surface");
        cmd.set(msg, "result", g_framework->AttachSurface(env, surface));
    } else if (cmdName == "resumeSurface") {
        g_framework->ResumeSurfaceRendering();
    } else if (cmdName == "detachSurface") {
        bool destroySurface = cmd.getBool(msg, "destroy");
        g_framework->DetachSurface(destroySurface);
    } else if (cmdName == "pauseSurface") {
        g_framework->PauseSurfaceRendering();
    } else if (cmdName == "changeSurface") {
        jobject surface = cmd.getObj(msg, "surface");
        int w = cmd.getInt(msg, "w");
        int h = cmd.getInt(msg, "h");
        g_framework->Resize(env, surface, w, h);
    } else if (cmdName == "scalePlus") {
        g_framework->Scale(::Framework::SCALE_MAG);
    } else if (cmdName == "scaleMinus") {
        g_framework->Scale(::Framework::SCALE_MIN);
    } else if (cmdName == "IsDrapeEngineCreated") {
        cmd.set(msg, "result", g_framework->IsDrapeEngineCreated());
    } else if (cmdName == "destroySurfaceOnDetach") {
        cmd.set(msg, "result", g_framework->DestroySurfaceOnDetach());
    } else if (cmdName == "locationState_switchToNextMode") {
        g_framework->SwitchMyPositionNextMode();
    } else if (cmdName == "locationState_getMode") {
        cmd.set(msg, "result", g_framework->GetMyPositionMode());
    } else if (cmdName == "locationState_setListener") {
        jobject listener = cmd.getObj(msg, "listener");
        g_framework->SetMyPositionModeListener(
                std::bind(&LocationStateModeChanged, std::placeholders::_1,
                          jni::make_global_ref(listener)));
    } else if (cmdName == "locationState_removeListener") {
        g_framework->SetMyPositionModeListener(location::TMyPositionModeChanged());
    } else if (cmdName == "locationState_setPendingTimeoutListener") {
        jobject listener = cmd.getObj(msg, "listener");
        g_framework->NativeFramework()->SetMyPositionPendingTimeoutListener(
                std::bind(&LocationPendingTimeout, jni::make_global_ref(listener)));
    } else if (cmdName == "locationState_removePendingTimeoutListener") {
        g_framework->SetMyPositionModeListener(location::TMyPositionModeChanged());
    } else if (cmdName == "locationState_onError") {
        g_framework->OnLocationError(cmd.getInt(msg, "errorCode"));
    } else if (cmdName == "locationState_onUpdated") {
        location::GpsInfo info;
        info.m_source = location::EAndroidNative;
        info.m_timestamp = static_cast<double>(cmd.getLong(msg, "time")) / 1000.0;
        info.m_latitude = cmd.getDouble(msg, "lat");
        info.m_longitude = cmd.getDouble(msg, "lon");
        float accuracy = cmd.getFloat(msg, "accuracy");
        if (accuracy > 0.0)
            info.m_horizontalAccuracy = accuracy;

        double altitude = cmd.getDouble(msg, "altitude");
        if (altitude != 0.0) {
            info.m_altitude = altitude;
            info.m_verticalAccuracy = accuracy;
        }

        float bearing = cmd.getFloat(msg, "bearing");
        if (bearing > 0.0)
            info.m_bearing = bearing;

        float speed = cmd.getFloat(msg, "speed");
        if (speed > 0.0)
            info.m_speedMpS = speed;

//        LOG_MEMORY_INFO();
        if (g_framework)
            g_framework->OnLocationUpdated(info);
        GpsTracker::Instance().OnLocationUpdated(info);
        g_framework->SetMyPositionModeListener(location::TMyPositionModeChanged());
    } else if (cmdName == "runFirstLaunchAnimation") {
        if (g_framework->NativeFramework()->GetDrapeEngine())
            g_framework->NativeFramework()->GetDrapeEngine()->RunFirstLaunchAnimation();
    } else if (cmdName == "cancelSearch") {
        g_framework->NativeFramework()->GetSearchAPI().CancelAllSearches();
    } else if (cmdName == "searchWay") {
        //沿途搜索
        std::vector<std::string> strPoints;
        std::string wayRoute = cmd.getStr(msg, "wayRoute");
        Stringsplit(wayRoute, ";", strPoints);
        Linestring route_line;
        for (auto strPoint: strPoints) {
            std::vector<std::string> spoint;
            Stringsplit(strPoint, ",", spoint);
            if (spoint.size() != 2) {

//                return;
            }
            boost::geometry::append(route_line, Point(stod(spoint[0]), stod(spoint[1])));
        }

        MultiLinestring mul_line;
        mul_line.push_back(route_line);

        boost::geometry::model::multi_polygon<Polygon> result1;

        boost::geometry::strategy::buffer::end_round end_strategy(36);
        boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(0.8);

        boost::geometry::strategy::buffer::side_straight side_strategy;
        boost::geometry::strategy::buffer::join_round join_strategy;
        boost::geometry::strategy::buffer::point_circle point_strategy;

        boost::geometry::buffer(mul_line, result1, distance_strategy, side_strategy, join_strategy,
                                end_strategy, point_strategy);

        if (result1.size() < 1) {
//            return ;
        } else {

        }

        Polygon polygon_buffer = result1[0];


        auto tmpMsg = env->NewGlobalRef(msg);
        auto onResults = [&, polygon_buffer, tmpMsg](search::Results const &results,
                                                     std::vector<search::ProductInfo> const &productInfo) {
            Polygon tmp_polygon_buffer = polygon_buffer;
            std::vector<search::Result> tmp_results;
            tmp_results.assign(results.begin(), results.end());
            //
            base::EraseIf(tmp_results,
                          [&](search::Result const &result_item) {
                              auto center = result_item.GetFeatureCenter();
                              ms::LatLon latLon = mercator::ToLatLon(center);
                              Point tmp(latLon.m_lon, latLon.m_lat);

                              return !boost::geometry::within(tmp, tmp_polygon_buffer);
                          });
            auto it = tmp_results.begin();
            auto array = json.createJSONArray();
            std::stringstream stream;
            stream << "[";
            while (it != tmp_results.end()) {
                stream << "{";
                std::string name = it->GetString();
                std::string address = it->GetAddress();
                double lat = 0;
                double lon = 0;
                if (it->HasPoint()) {
                    m2::PointD pt = it->GetFeatureCenter();
                    ms::LatLon latLon = mercator::ToLatLon(pt);
                    lat = latLon.m_lat;
                    lon = latLon.m_lon;
                }
                auto info = it->GetRankingInfo();
                double distance = info.m_distanceToPivot;
                stream << "\"name\":" << "\"" << name << "\",";
                stream << "\"address\":" << "\"" << address << "\",";
                stream << "\"lat\":" << "\"" << lat << "\",";
                stream << "\"lon\":" << "\"" << lon << "\",";
                stream << "\"distance\":" << "\"" << std::fixed << distance << "\"";
                stream << "}";
                //<<std::fixed
                if (it != results.end() - 1) {
                    stream << ",";
                }

                it++;
            }
            stream << "]";
            auto item = json.createJSONObject();
            json.setString(item, "data", stream.str().c_str());
            json.append(array, item);
            cmd.asyncCall(tmpMsg, array);

        };
        search::EverywhereSearchParams params;
        params.m_query = cmd.getStr(msg, "query");
        params.m_lat = cmd.getDouble(msg, "lat");
        params.m_lon = cmd.getDouble(msg, "lon");
        params.m_inputLocale = "zh_CN_#Hans";
        params.m_onResults = onResults;
        g_framework->NativeFramework()->GetSearchAPI().SearchEverywhere(params);

    } else if (cmdName == "search") {
        auto tmpMsg = env->NewGlobalRef(msg);
        auto onResults = [&, tmpMsg](search::Results const &results,
                                     std::vector<search::ProductInfo> const &productInfo) {
            auto it = results.begin();
            auto array = json.createJSONArray();
            std::stringstream stream;
            stream << "[";
            while (it != results.end()) {
                stream << "{";
                std::string name = it->GetString();
                std::string address = it->GetAddress();
                double lat = 0;
                double lon = 0;
                if (it->HasPoint()) {
                    m2::PointD pt = it->GetFeatureCenter();
                    ms::LatLon latLon = mercator::ToLatLon(pt);
                    lat = latLon.m_lat;
                    lon = latLon.m_lon;
                }
                auto info = it->GetRankingInfo();
                double distance = info.m_distanceToPivot;

                stream << "\"name\":" << "\"" << name << "\",";
                stream << "\"address\":" << "\"" << address << "\",";
                stream << "\"lat\":" << "\"" << lat << "\",";
                stream << "\"lon\":" << "\"" << lon << "\",";
                stream << "\"distance\":" << "\"" << distance << "\"";
                stream << "}";
                if (it != results.end() - 1) {
                    stream << ",";
                }

                it++;
            }
            stream << "]";


//            jobject jObject = jobject.Parse(stream.str());
            auto item = json.createJSONObject();
            json.setString(item, "data", stream.str().c_str());
            json.append(array, item);
            cmd.asyncCall(tmpMsg, array);
        };
        search::EverywhereSearchParams params;
        params.m_query = cmd.getStr(msg, "query");
        params.m_lat = cmd.getDouble(msg, "lat");
        params.m_lon = cmd.getDouble(msg, "lon");
        params.m_inputLocale = "zh_CN_#Hans";
        params.m_onResults = onResults;
        g_framework->NativeFramework()->GetSearchAPI().SearchEverywhere(params);
    } else if (cmdName == "LatLonToMapObject") {
        auto tmpMsg = env->NewGlobalRef(msg);
        double_t lat = cmd.getDouble(msg, "lat");
        double_t lon = cmd.getDouble(msg, "lon");
        m2::PointD mercator = mercator::FromLatLon(ms::LatLon(lat, lon));
        auto const featureID = g_framework->NativeFramework()->GetFeatureAtPoint(mercator);
        auto dataJson = json.createJSONObject();
        if (featureID.IsValid()) {
            osm::MapObject mapObject = g_framework->NativeFramework()->GetMapObjectByID(featureID);
            std::string  poiName = mapObject.GetDefaultName();
            std::string  poiAddress = mapObject.GetHouseNumber();
            std::string  nullStr ="";
            if ( strcmp( poiName.c_str(), nullStr.c_str() ) == 0 ){
                json.setString(dataJson, "poiName", "未知位置");
            } else{
                json.setString(dataJson, "poiName", mapObject.GetDefaultName());
            }
            if ( strcmp( poiAddress.c_str(), nullStr.c_str() ) == 0 ){
                json.setString(dataJson, "poiName", "未获取到地址");
            }else{
                json.setString(dataJson, "poiAddress", mapObject.GetHouseNumber());
            }
        } else {
            json.setString(dataJson, "poiName", "未知位置");
            json.setString(dataJson, "poiAddress", "未获取到地址");
        }
        cmd.asyncCall(tmpMsg, dataJson);
    } else if (cmdName == "ScreenToMapObject") {

        auto tmpMsg = env->NewGlobalRef(msg);
        double_t screenX = cmd.getDouble(msg, "screenX");
        double_t screenY = cmd.getDouble(msg, "screenY");
        m2::PointD mercator = g_framework->NativeFramework()->PtoG(m2::PointD(screenX, screenY));
        std::double_t mercatorX = mercator.x;
        std::double_t mercatorY = mercator.y;
        auto dataJson = json.createJSONObject();
        m2::PointD tmpMercator = m2::PointD(mercatorX, mercatorY);
        ms::LatLon latLon = mercator::ToLatLon(tmpMercator);
        json.setDouble(dataJson, "lat", latLon.m_lat);
        json.setDouble(dataJson, "lon", latLon.m_lon);
        auto const featureID = g_framework->NativeFramework()->GetFeatureAtPoint(
                m2::PointD(mercatorX, mercatorY));
        if (featureID.IsValid()) {
            osm::MapObject mapObject = g_framework->NativeFramework()->GetMapObjectByID(featureID);
            std::string  poiName = mapObject.GetDefaultName();
            std::string  poiAddress = mapObject.GetHouseNumber();
            std::string  nullStr ="";
            if ( strcmp( poiName.c_str(), nullStr.c_str() ) == 0 ){
                json.setString(dataJson, "poiName", "未知位置");
            } else{
                json.setString(dataJson, "poiName", mapObject.GetDefaultName());
            }
            if ( strcmp( poiAddress.c_str(), nullStr.c_str() ) == 0 ){
                json.setString(dataJson, "poiName", "未获取到地址");
            }else{
                json.setString(dataJson, "poiAddress", mapObject.GetHouseNumber());
            }
        } else {
            json.setString(dataJson, "poiName", "未知位置");
            json.setString(dataJson, "poiAddress", "未获取到地址");
        }
        cmd.asyncCall(tmpMsg, dataJson);
    } else if (cmdName == "PtoG") {
//        屏幕坐标转墨卡托
        auto tmpMsg = env->NewGlobalRef(msg);
        double_t screenX = cmd.getDouble(msg, "screenX");
        double_t screenY = cmd.getDouble(msg, "screenY");
        m2::PointD mercator = g_framework->NativeFramework()->PtoG(m2::PointD(screenX, screenY));
        auto dataJson = json.createJSONObject();
        json.setDouble(dataJson, "mercatorX", mercator.x);
        json.setDouble(dataJson, "mercatorY", mercator.y);
        cmd.asyncCall(tmpMsg, dataJson);
    } else if (cmdName == "ToLatLon") {
//        墨卡托转wgs84
        auto tmpMsg = env->NewGlobalRef(msg);
        double_t mercatorX = cmd.getDouble(msg, "mercatorX");
        double_t mercatorY = cmd.getDouble(msg, "mercatorY");
        m2::PointD tmpMercator = m2::PointD(mercatorX, mercatorY);
        ms::LatLon latLon = mercator::ToLatLon(tmpMercator);
        auto dataJson = json.createJSONObject();
        json.setDouble(dataJson, "lat", latLon.m_lat);
        json.setDouble(dataJson, "lon", latLon.m_lon);
        cmd.asyncCall(tmpMsg, dataJson);
    } else if (cmdName == "FromLatLon") {
//        wgs84转墨卡托
        auto tmpMsg = env->NewGlobalRef(msg);
        double_t lat = cmd.getDouble(msg, "lat");
        double_t lon = cmd.getDouble(msg, "lon");
        m2::PointD mercator = mercator::FromLatLon(ms::LatLon(lat, lon));
        auto dataJson = json.createJSONObject();
        json.setDouble(dataJson, "mercatorY", mercator.y);
        json.setDouble(dataJson, "mercatorX", mercator.x);
        cmd.asyncCall(tmpMsg, dataJson);

    } else if (cmdName == "GetFeatureID") {
        double_t mercatorX = cmd.getDouble(msg, "mercatorX");
        double_t mercatorY = cmd.getDouble(msg, "mercatorY");
        auto const featureID = g_framework->NativeFramework()->GetFeatureAtPoint(
                m2::PointD(mercatorX, mercatorY));
    } else if (cmdName == "GetMapObject") {
        auto tmpMsg = env->NewGlobalRef(msg);
        std::double_t mercatorX = cmd.getDouble(msg, "mercatorX");
        std::double_t mercatorY = cmd.getDouble(msg, "mercatorY");
        auto dataJson = json.createJSONObject();
        json.setDouble(dataJson, "mercatorX", mercatorX);
        json.setDouble(dataJson, "mercatorY", mercatorY);
        //gandong
//        m2::PointD mercator = mercator::FromLatLon(ms::LatLon( 39.909291795684,116.388100209744));
        auto const featureID = g_framework->NativeFramework()->GetFeatureAtPoint(
                m2::PointD(mercatorX, mercatorY));
//        auto const featureID = g_framework->NativeFramework()->GetFeatureAtPoint(
//                mercator);
        if (featureID.IsValid()) {
            osm::MapObject mapObject = g_framework->NativeFramework()->GetMapObjectByID(featureID);
            std::string  poiName = mapObject.GetDefaultName();
            std::string  poiAddress = mapObject.GetHouseNumber();
            std::string  nullStr ="";
            if ( strcmp( poiName.c_str(), nullStr.c_str() ) == 0 ){
                json.setString(dataJson, "poiName", "未知位置");
            } else{
                json.setString(dataJson, "poiName", mapObject.GetDefaultName());
            }
            if ( strcmp( poiAddress.c_str(), nullStr.c_str() ) == 0 ){
                json.setString(dataJson, "poiName", "未获取到地址");
            }else{
                json.setString(dataJson, "poiAddress", mapObject.GetHouseNumber());
            }
        } else {
            json.setString(dataJson, "poiName", "未知位置");
            json.setString(dataJson, "poiAddress", "未获取到地址");
        }
        cmd.asyncCall(tmpMsg, dataJson);
//        env->DeleteGlobalRef(tmpMsg);
    } else if (cmdName == "ClickListener") {
        auto touchListener = [](df::TapInfo const &tapInfo) {
//            auto tmpMsg = env->NewGlobalRef(msg);
            bool IsLong = tapInfo.m_isLong;
            m2::PointD mercator = tapInfo.m_mercator;
            auto dataJson = json.createJSONObject();
            ms::LatLon latLon = mercator::ToLatLon(mercator);
            ms::LatLon latLon1 = mercator::ToLatLon(mercator);
            json.setDouble(dataJson, "mercatorX", mercator.x);

            json.setDouble(dataJson, "mercatorY", mercator.y);
//            cmd.asyncCall(tmpMsg, dataJson);
        };

        auto nativeFramework = g_framework->NativeFramework();
        auto drapeEngine = nativeFramework->GetDrapeEngine();
        drapeEngine->SetTapEventInfoListener(touchListener);
    } else if (cmdName == "nativeSetupWidget") {
//        屏幕坐标转墨卡托
        m2::PointD mercator = g_framework->NativeFramework()->PtoG(m2::PointD(345, 887));
//        116.39126521640469  43.59062286906408 天安门
        //北京大学
//                    p.put("x", 116.30959463350172);
//                    p.put("y", 43.69950312152136);
        m2::PointD tam = m2::PointD(116.39126521640469, 43.59062286906408);
//        墨卡托转wgs84
        ms::LatLon latLon = mercator::ToLatLon(tam);
//        wgs84获取FeatureID
        auto const id = g_framework->NativeFramework()->GetFeatureAtPoint(
                mercator::FromLatLon(latLon));

        osm::MapObject mapObject = g_framework->NativeFramework()->GetMapObjectByID(id);
        std::string name = mapObject.GetDefaultName();
        osm::MapObject mapObject1 = g_framework->NativeFramework()->GetMapObjectByID(id);

    } else if (cmdName == "setViewCenter") {
        double lat = cmd.getDouble(msg, "lat");
        double lon = cmd.getDouble(msg, "lon");
        int zoom = cmd.getInt(msg, "zoom");
        if (lat <= 0.0 || lon <= 0.0) {
            g_framework->SwitchMyPositionNextMode();
        } else {
            m2::PointD mercatorPT = mercator::FromLatLon(lat, lon);
            auto engine = g_framework->NativeFramework()->GetDrapeEngine();
            if (engine)
                engine->SetModelViewCenter(mercatorPT, zoom, false, false);

        }
    } else if (cmdName == "centerPoints") {
        auto engine = g_framework->NativeFramework()->GetDrapeEngine();
        jobject array = cmd.getObj(msg, "points");
        int length = json.length(array);
        std::vector<m2::PointD> points;
        m2::RectD routeRect;
        for (int i = 0; i < length; i += 2) {
            double x = json.getDouble(array, i);
            double y = json.getDouble(array, i + 1);
            m2::PointD mercatorPoint = mercator::FromLatLon(y, x);
            routeRect.Add(mercatorPoint);
        }
        routeRect.Scale(1.5);
        engine->SetModelViewRect(routeRect,
                                 true /* applyRotation */, -1 /* zoom */, true /* isAnim */,
                                 true /* useVisibleViewport */);
    } else if (cmdName == "setMapStyle") {
//        ENGINE().clearRoutes();
        std::string style = cmd.getStr(msg, "style");
        if (style == "clear") {
            g_framework->NativeFramework()->SetMapStyle(MapStyle::MapStyleClear);
        } else if (style == "dark") {
            g_framework->NativeFramework()->SetMapStyle(MapStyle::MapStyleDark);
        } else if (style == "camouflage") {
            g_framework->NativeFramework()->SetMapStyle(MapStyle::MapStyleCamouflage);

        }

    } else if (cmdName == "route") {
        ENGINE().clearRoutes();
        jobject array = cmd.getObj(msg, "points");
        std::string type = cmd.getStr(msg, "type");
        int length = json.length(array);
        std::vector<m2::PointD> points;
        for (int i = 0; i < length; i++) {
            jobject o = json.getJSONObject(array, i);
            m2::PointD mercator = mercator::FromLatLon(
                    ms::LatLon(json.getDouble(o, "lat"), json.getDouble(o, "lon")));
            points.push_back(mercator);
            RouteMarkData data;
            data.m_title = json.getString(o, "name");
            data.m_subTitle = "";
            if (i == 0) {
                data.m_pointType = RouteMarkType::Start;
            } else if (i == length - 1) {
                data.m_pointType = RouteMarkType::Finish;
            } else {
                data.m_pointType = RouteMarkType::Intermediate;
            }
            data.m_intermediateIndex = static_cast<size_t>(0);
//            data.
            if (json.getBool(o, "isMyPosition")) {
                data.m_isMyPosition = static_cast<bool>(true);
            } else {
                data.m_isMyPosition = static_cast<bool>(false);
            }
            data.m_position = mercator;
            if (type == "Vehicle") {
                g_framework->NativeFramework()->GetRoutingManager().SetRouter(
                        routing::RouterType::Vehicle);
            } else if (type == "Pedestrian") {
                g_framework->NativeFramework()->GetRoutingManager().SetRouter(
                        routing::RouterType::Pedestrian);
            }
            g_framework->NativeFramework()->GetRoutingManager().AddRoutePoint(std::move(data));
        }
        auto tmpMsg = env->NewGlobalRef(msg);
        auto result = env->NewGlobalRef(json.createJSONObject());
        ENGINE().buildRoute(points, [&, result, tmpMsg](const std::string &code,
                                                        const std::vector<std::string> &routeIds) {
            auto routeIdList = json.createJSONArray();
            for (int i = 0; i < routeIds.size(); i++) {
                json.append(routeIdList, json.toJNIString(routeIds[i]));
            }
            json.setObject(result, "routeIdList", routeIdList);
            cmd.asyncCall(tmpMsg, result);
        });



    } else if (cmdName == "getAllRouteInfo") {
        auto tmpMsg = env->NewGlobalRef(msg);
        jobject array = cmd.getObj(msg, "allRouteInfo");
        int length = json.length(array);
        auto routeInfoList = json.createJSONArray();
        for (int i = 0; i < length; i++) {
            jobject routeIdobj = json.getJSONObject(array, i);
            std::string routeId = strings::to_string(routeIdobj);
            std::string routeInfo = ENGINE().getRouteInfo(routeId);
            auto result = env->NewGlobalRef(json.createJSONObject());
            json.setObject(result, "RouteInfo", json.toJNIString(routeInfo));
            double routeDistance = ENGINE().getRouteDistance(routeId);
            json.setObject(result, "routeDistance",
                           json.toJNIString(std::to_string(routeDistance)));
            double time = ENGINE().getRouteTime(routeId);
            json.setObject(result, "routeTime", json.toJNIString(std::to_string(time)));

            json.setObject(routeInfoList, "routeIdList", result);

        }
        cmd.asyncCall(tmpMsg, routeInfoList);
    } else if (cmdName == "getRouteInfo") {
        auto tmpMsg = env->NewGlobalRef(msg);
        std::string routeId = cmd.getStr(msg, "routeId");
        std::string routeInfo = ENGINE().getRouteInfo(routeId);
        auto result = env->NewGlobalRef(json.createJSONObject());
        json.setObject(result, "routeInfo", json.toJNIString(routeInfo));
        double routeDistance = ENGINE().getRouteDistance(routeId);
        json.setObject(result, "routeDistance", json.toJNIString(std::to_string(routeDistance)));
        double time = ENGINE().getRouteTime(routeId);
        json.setObject(result, "routeTime", json.toJNIString(std::to_string(time)));
        cmd.asyncCall(tmpMsg, result);
    } else if (cmdName == "getRouteDistance") {
        auto tmpMsg = env->NewGlobalRef(msg);
        std::string routeId = cmd.getStr(msg, "routeId");
        double routeDistance = ENGINE().getRouteDistance(routeId);
        auto result = env->NewGlobalRef(json.createJSONObject());
        json.setObject(result, "routeDistance", json.toJNIString(std::to_string(routeDistance)));
        cmd.asyncCall(tmpMsg, result);
    } else if (cmdName == "getRouteTime") {
        auto tmpMsg = env->NewGlobalRef(msg);
        std::string routeId = cmd.getStr(msg, "routeId");
        double time = ENGINE().getRouteTime(routeId);
        auto result = env->NewGlobalRef(json.createJSONObject());
        json.setObject(result, "routeTime", json.toJNIString(std::to_string(time)));
        cmd.asyncCall(tmpMsg, result);
    } else if (cmdName == "hideRoute") {
        std::string routeId = cmd.getStr(msg, "routeId");
        ENGINE().hideRoute(routeId);
    } else if (cmdName == "showRoute") {
        std::string routeId = cmd.getStr(msg, "routeId");
        std::string fillColor = cmd.getStr(msg, "fillColor");
        std::string outlineColor = cmd.getStr(msg, "outlineColor");
        ENGINE().showRoute(routeId, fillColor, outlineColor);
    } else if (cmdName == "followRoute") {
        std::string routeId = cmd.getStr(msg, "routeId");
//        g_framework->NativeFramework()->GetRoutingManager().FollowRoute();
        ENGINE().enterFollowRoute(routeId);
    } else if (cmdName == "exitRoute") {
        ENGINE().exitFollowRoute();
    } else if (cmdName == "updatePreviewMode") {
        std::string routeId = cmd.getStr(msg, "routeId");
        ENGINE().updatePreviewMode(routeId);
    } else if (cmdName == "updatePreviewModeAll") {
        ENGINE().updatePreviewModeAll();
    } else if (cmdName == "removeRoute") {
        ENGINE().removeRoute(true);
    } else if (cmdName == "closeRouting") {
        ENGINE().closeRouting(true);
    } else if (cmdName == "removeCustomMark") {
        jobject array = cmd.getObj(msg, "customMarkList");
        int length = json.length(array);
        std::vector<std::string> idList;
        for (int i = 0; i < length; i++) {
            std::string markIdStr = json.getString(array, i);
            idList.push_back(markIdStr);
        }
        g_framework->NativeFramework()->GetDrapeApi().RemoveCustomMarkBatch(idList);
    } else if (cmdName == "addCustomMark") {
        std::string id = cmd.getStr(msg, "id");
        std::string path = cmd.getStr(msg, "path");
        std::string text = cmd.getStr(msg, "text");
        dp::Color color = stringToDpColor(cmd.getStr(msg, "color"));
        int fontSize = cmd.getInt(msg, "fontSize");
//        double lon = cmd.getDouble(msg, "lon");
//        double lat = cmd.getDouble(msg, "lat");
        m2::PointD mercator = mercator::FromLatLon(
                ms::LatLon(cmd.getDouble(msg, "lat"), cmd.getDouble(msg, "lon")));
        int diaplayWidth = cmd.getInt(msg, "diaplayWidth");
        int diaplayHeight = cmd.getInt(msg, "diaplayHeight");
//        m2::PointD center = {lon, lat};
        m2::PointD markSize = {(double)diaplayWidth, (double)diaplayHeight};
        df::DrapeApiCustomMarkData markData(id, mercator, markSize, text, path, color, fontSize);
        g_framework->NativeFramework()->GetDrapeApi().AddCustomMark(id, markData);
        cmd.set(msg, "result", id.c_str());
    } else if (cmdName == "addDrawItem") {
        std::string type = cmd.getStr(msg, "type");
        if (type == "line") {
            std::string id = cmd.getStr(msg, "id");
            dp::Color color = stringToDpColor(cmd.getStr(msg, "color"));
//            auto lineMask = userMarks.CreateUserLineMark();
            jobject array = cmd.getObj(msg, "points");
            int length = json.length(array);
            std::vector<m2::PointD> points;
            for (int i = 0; i < length; i += 2) {
                double x = json.getDouble(array, i);
                double y = json.getDouble(array, i + 1);
                m2::PointD mercatorPoint = mercator::FromLatLon(y, x);
//                lineMask->addPoint(mercatorPoint.x,mercatorPoint. y);
                points.push_back(mercatorPoint);
            }
//            lineMask->SetColor(color);
            df::DrapeApiLineData lineData(points, color);
            double width = cmd.getDouble(msg, "width");
            lineData.Width(width);
            g_framework->NativeFramework()->GetDrapeApi().AddLine(id, lineData);

            long number = std::atoi(id.c_str());
//            long  long  number = strtoll(id, NULL, 10);
            cmd.set(msg, "result", number);
//            userMarks.notifyChanges();
        } else if (type == "coloredPoint") {
            std::string icon = cmd.getStr(msg, "icon");
            double x = cmd.getDouble(msg, "x");
            double y = cmd.getDouble(msg, "y");
            m2::PointD mercatorPoint = mercator::FromLatLon(y, x);
            auto mark = userMarks.CreateUserMark<ColoredMarkPoint>(mercatorPoint);
            mark->SetColor(dp::Color(200, 100, 240, 255));
            mark->SetRadius(18.0f);
            long long id = mark->GetId();
            cmd.set(msg, "result", id);
            userMarks.notifyChanges();
        } else if (type == "textPoint") {
            double x = cmd.getDouble(msg, "x");
            double y = cmd.getDouble(msg, "y");
            m2::PointD mercatorPoint = mercator::FromLatLon(y, x);
            auto mark = userMarks.CreateUserMark<RouteMarkPoint>(mercatorPoint);
            long long id = mark->GetId();
            std::string title = cmd.getStr(msg, "title");
            RouteMarkData data;
            data.m_title = title;
            mark->SetMarkData(std::move(data));
            cmd.set(msg, "result", id);
            userMarks.notifyChanges();
        } else if (type == "point") {
            std::string icon = cmd.getStr(msg, "icon");
            double x = cmd.getDouble(msg, "x");
            double y = cmd.getDouble(msg, "y");
            m2::PointD mercatorPoint = mercator::FromLatLon(y, x);

//            auto mark1 =  userMarks.CreateUserMark<ApiMarkPoint>(mercatorPoint);
//            auto mark2 =  userMarks.CreateUserMark<RouteMarkPoint>(mercatorPoint);
            auto mark = userMarks.CreateUserMark<MyUserMark>(mercatorPoint);
//            auto mark = userMarks.CreateUserMark<ColoredMarkPoint>(mercatorPoint);
//            mark->SetColor(dp::Color(200, 100, 240, 255));
//            auto * newPoint = userMarks.CreateUserMark<DebugMarkPoint>(m2::PointD(x, y));
//            mark->SetRadius(18.0f);
//            std::string a("route-point-finish");
            mark->setIcon(icon);
            std::string name = "测试点名称";
//            mark1->SetName(name);
//            long long id = mark->GetId();
            long long id = mark->GetId();
            RouteMarkData data;
            MyUserMarkData data1;
            data1.m_title = "测试点名称";
            data.m_title = "测试点名称";
//            data.m_pointType = RouteMarkType::Green;
//            mark2->SetMarkData(std::move(data));
//            mark2->SetRoutePointType(RouteMarkType::Green);
//            mark->SetMarkData(std::move(data1));
//            mark1.
            cmd.set(msg, "result", id);
//            cmd.set(msg, "result", 10000020100);
            userMarks.notifyChanges();
//            newPoint.notifyChanges();
        }
    } else if (cmdName == "updateDrawItem") {
        g_framework->NativeFramework()->GetDrapeApi().Invalidate();
    } else if (cmdName == "removeLineItem") {
        jobject array = cmd.getObj(msg, "lineIdList");
        int length = json.length(array);
        for (int i = 0; i < length; i++) {
            std::string lineIdStr = json.getString(array, i);
            g_framework->NativeFramework()->GetDrapeApi().RemoveLine(lineIdStr);
        }
    } else if (cmdName == "removeDrawItem") {
        std::string id = cmd.getStr(msg, "id");
        g_framework->NativeFramework()->GetDrapeApi().RemoveLine(id);
    } else if (cmdName == "removeMarkItem") {
        jobject array = cmd.getObj(msg, "markIdList");
        int length = json.length(array);
        for (int i = 0; i < length; i++) {
            std::string markIdStr = json.getString(array, i);
            long long markId;
            std::stringstream sstr;
            sstr.clear();
            sstr << markIdStr;
            sstr >> markId;
            userMarks.RemoveUserPointMark(markId);
        }


    } else if (cmdName == "updateMarkItem") {
        double x = cmd.getDouble(msg, "x");
        double y = cmd.getDouble(msg, "y");
//        long  long id = cmd.getLong(msg, "markId");//mark->GetId();
        std::string id = cmd.getStr(msg, "markId");
        std::string icon = cmd.getStr(msg, "icon");
        long long markId;
        std::stringstream sstr;
        sstr.clear();
        sstr << id;
        sstr >> markId;
        auto mark = (MyUserMark *) userMarks.GetUserPointMark(markId);
        cmd.set(msg, "result", markId);
        m2::PointD mercatorPoint = mercator::FromLatLon(y, x);
        mark->setPos(mercatorPoint);
        mark->setIcon(icon);
        userMarks.UpdateUserPointMark(markId);
    } else if (cmdName == "getZoomlevel") {
       int zoomlevel = g_framework->NativeFramework()->GetDrawScale();
       cmd.set(msg, "zoomlevel", zoomlevel);
    }else if (cmdName == "setMyDefaultPosition") {
        double lat = cmd.getDouble(msg, "lat");
        double lon = cmd.getDouble(msg, "lon");
        m2::PointD mercatorPT = mercator::FromLatLon(lat, lon);
        g_framework->SetMyDefaultPosition(mercatorPT);
    }
    return nullptr;
}
RoutingManager::LoadRouteHandler g_loadRouteHandler;
JNIEXPORT jint
JNICALL
Java_com_ftmap_maps_FTMap_nativeGetRouter(JNIEnv *env, jclass) {
    return static_cast<jint>(g_framework->GetRoutingManager().GetRouter());
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeRunFirstLaunchAnimation(JNIEnv *env, jclass) {
    frm()->RunFirstLaunchAnimation();
}
JNIEXPORT jboolean

JNICALL
Java_com_ftmap_maps_FTMap_nativeIsRouteFinished(JNIEnv *env, jclass) {
    return frm()->GetRoutingManager().IsRouteFinished();
}

JNIEXPORT jobject

JNICALL
Java_com_ftmap_maps_FTMap_nativeGetRouteFollowingInfo(JNIEnv *env, jclass) {
    ::Framework *fr = frm();
    if (!fr->GetRoutingManager().IsRoutingActive())
        return nullptr;

    routing::FollowingInfo info;
    fr->GetRoutingManager().GetRouteFollowingInfo(info);
    if (!info.IsValid())
        return nullptr;
//com.ftmap.maps
    static jclass const klass = jni::GetGlobalClassRef(env, "com/ftmap/maps/RoutingInfo");
    // Java signature : RoutingInfo(String distToTarget, String units, String distTurn, String
    //                              turnSuffix, String currentStreet, String nextStreet,
    //                              double completionPercent, int vehicleTurnOrdinal, int
    //                              vehicleNextTurnOrdinal, int pedestrianTurnOrdinal, int exitNum,
    //                              int totalTime, SingleLaneInfo[] lanes)
    static jmethodID const ctorRouteInfoID =
            jni::GetConstructorID(env, klass,
                                  "(Ljava/lang/String;Ljava/lang/String;"
                                  "Ljava/lang/String;Ljava/lang/String;"
                                  "Ljava/lang/String;Ljava/lang/String;DIIIII"
                                  "[Lcom/ftmap/maps/SingleLaneInfo;ZZ)V");

    std::vector<routing::FollowingInfo::SingleLaneInfoClient> const &lanes = info.m_lanes;
    jobjectArray jLanes = nullptr;
    if (!lanes.empty()) {
        static jclass const laneClass = jni::GetGlobalClassRef(env,
                                                               "com/ftmap/maps/SingleLaneInfo");
        auto const lanesSize = static_cast<jsize>(lanes.size());
        jLanes = env->NewObjectArray(lanesSize, laneClass, nullptr);
        ASSERT(jLanes, (jni::DescribeException()));
        static jmethodID const ctorSingleLaneInfoID = jni::GetConstructorID(env, laneClass,
                                                                            "([BZ)V");

        for (jsize j = 0; j < lanesSize; ++j) {
            auto const laneSize = static_cast<jsize>(lanes[j].m_lane.size());
            jni::TScopedLocalByteArrayRef singleLane(env, env->NewByteArray(laneSize));
            ASSERT(singleLane.get(), (jni::DescribeException()));
            env->SetByteArrayRegion(singleLane.get(), 0, laneSize, lanes[j].m_lane.data());

            jni::TScopedLocalRef singleLaneInfo(
                    env, env->NewObject(laneClass, ctorSingleLaneInfoID, singleLane.get(),
                                        lanes[j].m_isRecommended));
            ASSERT(singleLaneInfo.get(), (jni::DescribeException()));
            env->SetObjectArrayElement(jLanes, j, singleLaneInfo.get());
        }
    }

    auto const &rm = frm()->GetRoutingManager();
    auto const isSpeedCamLimitExceeded = rm.IsRoutingActive() ? rm.IsSpeedLimitExceeded() : false;
    auto const shouldPlaySignal = frm()->GetRoutingManager().GetSpeedCamManager().ShouldPlayBeepSignal();
    jobject const result = env->NewObject(
            klass, ctorRouteInfoID, jni::ToJavaString(env, info.m_distToTarget),
            jni::ToJavaString(env, info.m_targetUnitsSuffix),
            jni::ToJavaString(env, info.m_distToTurn),
            jni::ToJavaString(env, info.m_turnUnitsSuffix),
            jni::ToJavaString(env, info.m_sourceName),
            jni::ToJavaString(env, info.m_displayedStreetName), info.m_completionPercent,
            info.m_turn,
            info.m_nextTurn, info.m_pedestrianTurn, info.m_exitNum, info.m_time, jLanes,
            static_cast<jboolean>(isSpeedCamLimitExceeded),
            static_cast<jboolean>(shouldPlaySignal));
    ASSERT(result, (jni::DescribeException()));
    return result;
}

JNIEXPORT jobject

JNICALL
Java_com_ftmap_maps_FTMap_nativeGetTransitRouteInfo(JNIEnv *env, jclass) {
    auto const routeInfo = frm()->GetRoutingManager().GetTransitRouteInfo();

    static jclass const transitStepClass = jni::GetGlobalClassRef(env,
                                                                  "com/ftmap/maps/TransitStepInfo");
    // Java signature : TransitStepInfo(@TransitType int type, @Nullable String distance, @Nullable String distanceUnits,
    //                                  int timeInSec, @Nullable String number, int color, int intermediateIndex)
    static jmethodID const transitStepConstructor = jni::GetConstructorID(env, transitStepClass,
                                                                          "(ILjava/lang/String;Ljava/lang/String;ILjava/lang/String;II)V");

    jni::TScopedLocalRef const steps(env, jni::ToJavaArray(env, transitStepClass,
                                                           routeInfo.m_steps,
                                                           [&](JNIEnv *jEnv,
                                                               TransitStepInfo const &stepInfo) {
                                                               jni::TScopedLocalRef const distance(
                                                                       env, jni::ToJavaString(env,
                                                                                              stepInfo.m_distanceStr));
                                                               jni::TScopedLocalRef const distanceUnits(
                                                                       env, jni::ToJavaString(env,
                                                                                              stepInfo.m_distanceUnitsSuffix));
                                                               jni::TScopedLocalRef const number(
                                                                       env, jni::ToJavaString(env,
                                                                                              stepInfo.m_number));
                                                               return env->NewObject(
                                                                       transitStepClass,
                                                                       transitStepConstructor,
                                                                       static_cast<jint>(stepInfo.m_type),
                                                                       distance.get(),
                                                                       distanceUnits.get(),
                                                                       static_cast<jint>(stepInfo.m_timeInSec),
                                                                       number.get(),
                                                                       static_cast<jint>(stepInfo.m_colorARGB),
                                                                       static_cast<jint>(stepInfo.m_intermediateIndex));
                                                           }));
//com.ftmap.maps
    static jclass const transitRouteInfoClass = jni::GetGlobalClassRef(env,
                                                                       "com/ftmap/maps/TransitRouteInfo");
    // Java signature : TransitRouteInfo(@NonNull String totalDistance, @NonNull String totalDistanceUnits, int totalTimeInSec,
    //                                   @NonNull String totalPedestrianDistance, @NonNull String totalPedestrianDistanceUnits,
    //                                   int totalPedestrianTimeInSec, @NonNull TransitStepInfo[] steps)

    //
    static jmethodID const transitRouteInfoConstructor = jni::GetConstructorID(env,
                                                                               transitRouteInfoClass,
                                                                               "(Ljava/lang/String;Ljava/lang/String;I"
                                                                               "Ljava/lang/String;Ljava/lang/String;I"
                                                                               "[Lcom/ftmap/maps/TransitStepInfo;)V");
    jni::TScopedLocalRef const distance(env, jni::ToJavaString(env, routeInfo.m_totalDistanceStr));
    jni::TScopedLocalRef const distanceUnits(env, jni::ToJavaString(env,
                                                                    routeInfo.m_totalDistanceUnitsSuffix));
    jni::TScopedLocalRef const distancePedestrian(env, jni::ToJavaString(env,
                                                                         routeInfo.m_totalPedestrianDistanceStr));
    jni::TScopedLocalRef const distancePedestrianUnits(env, jni::ToJavaString(env,
                                                                              routeInfo.m_totalPedestrianUnitsSuffix));
    return env->NewObject(transitRouteInfoClass, transitRouteInfoConstructor,
                          distance.get(), distanceUnits.get(),
                          static_cast<jint>(routeInfo.m_totalTimeInSec),
                          distancePedestrian.get(), distancePedestrianUnits.get(),
                          static_cast<jint>(routeInfo.m_totalPedestrianTimeInSec),
                          steps.get());
}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeDisableFollowing(JNIEnv
                                                 *env, jclass) {
    frm()->GetRoutingManager().

            DisableFollowMode();

}

JNIEXPORT jint

JNICALL
Java_com_ftmap_maps_FTMap_nativeGetLastUsedRouter(JNIEnv *env, jclass) {
    return static_cast<jint>(g_framework->GetRoutingManager().GetLastUsedRouter());
}

JNIEXPORT jint

JNICALL
Java_com_ftmap_maps_FTMap_nativeInvalidRoutePointsTransactionId(JNIEnv *env, jclass) {
    return frm()->GetRoutingManager().InvalidRoutePointsTransactionId();
}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeSetRoutingListener(JNIEnv
                                                   *env, jclass,
                                                   jobject listener
) {
    CHECK(g_framework,
          ("Framework isn't created yet!"));
    auto rf = jni::make_global_ref(listener);
    frm()->GetRoutingManager().SetRouteBuildingListener(
            [rf](
                    routing::RouterResultCode e, storage::CountriesSet
            const &countries) {
                CallRoutingListener(rf,
                                    static_cast
                                            <int>(e), countries
                );
            });
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeSetRouteProgressListener(JNIEnv
                                                         *env, jclass,
                                                         jobject listener
) {
    CHECK(g_framework,
          ("Framework isn't created yet!"));
    frm()->GetRoutingManager().
            SetRouteProgressListener(
            bind(&CallRouteProgressListener, jni::make_global_ref(listener), std::placeholders::_1)
    );
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeSetRoutingRecommendationListener(JNIEnv
                                                                 *env, jclass,
                                                                 jobject listener
) {
    CHECK(g_framework,
          ("Framework isn't created yet!"));
    frm()->GetRoutingManager().
            SetRouteRecommendationListener(
            bind(&CallRouteRecommendationListener, jni::make_global_ref(listener),
                 std::placeholders::_1)
    );
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeSetRoutingLoadPointsListener(
        JNIEnv
        *, jclass,
        jobject listener
) {
    CHECK(g_framework,
          ("Framework isn't created yet!"));
    if (listener != nullptr)
        g_loadRouteHandler = bind(&CallSetRoutingLoadPointsListener, jni::make_global_ref(listener),
                                  std::placeholders::_1);
    else
        g_loadRouteHandler = nullptr;
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeRemoveRoute(JNIEnv
                                            *env, jclass) {
    frm()->GetRoutingManager().RemoveRoute(false /* deactivateFollowing */);
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeBuildRoute(JNIEnv
                                           *env, jclass) {
    frm()->GetRoutingManager().

            BuildRoute();

}


JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeLoadRoutePoints(JNIEnv
                                                *, jclass) {
    frm()->GetRoutingManager().
            LoadRoutePoints(g_loadRouteHandler);
}
JNIEXPORT jboolean

JNICALL
Java_com_ftmap_maps_FTMap_nativeHasSavedRoutePoints(JNIEnv *, jclass) {
    return frm()->GetRoutingManager().HasSavedRoutePoints();
}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeSaveRoutePoints(JNIEnv
                                                *, jclass) {
    frm()->GetRoutingManager().

            SaveRoutePoints();

}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeDeleteSavedRoutePoints(JNIEnv
                                                       *, jclass) {
    frm()->GetRoutingManager().

            DeleteSavedRoutePoints();

}
JNIEXPORT jint
JNICALL
Java_com_ftmap_maps_FTMap_nativeGetBestRouter(JNIEnv *env, jclass,
                                              jdouble
                                              srcLat,
                                              jdouble srcLon,
                                              jdouble
                                              dstLat,
                                              jdouble dstLon
) {
    return static_cast

            <jint> (frm()

                    ->

                            GetRoutingManager()

                    .
                            GetBestRouter(
                            mercator::FromLatLon(srcLat, srcLon),
                            mercator::FromLatLon(dstLat, dstLon)
                    ));
}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeFollowRoute(JNIEnv
                                            *env, jclass) {
    frm()->GetRoutingManager().

            FollowRoute();

}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeRemoveRoutePoint(JNIEnv
                                                 *env, jclass,
                                                 jint markType, jint
                                                 intermediateIndex) {
    frm()->GetRoutingManager().RemoveRoutePoint(static_cast
                                                        <RouteMarkType>(markType),
                                                static_cast
                                                        <size_t>(intermediateIndex)
    );
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeRemoveIntermediateRoutePoints(JNIEnv
                                                              *env, jclass) {
    frm()->GetRoutingManager().

            RemoveIntermediateRoutePoints();

}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeSetRouter(JNIEnv
                                          *env, jclass,
                                          jint routerType
) {
    using Type = routing::RouterType;
    Type type = Type::Vehicle;
    switch (routerType) {
        case 0:
            break;
        case 1:
            type = Type::Pedestrian;
            break;
        case 2:
            type = Type::Bicycle;
            break;
        case 3:
            type = Type::Transit;
            break;
        default:
            assert(false);
            break;
    }
    g_framework->

                    GetRoutingManager()

            .
                    SetRouter(type);
}
JNIEXPORT jboolean

JNICALL
Java_com_ftmap_maps_FTMap_nativeCouldAddIntermediatePoint(JNIEnv *env, jclass) {
    return frm()->GetRoutingManager().CouldAddIntermediatePoint();
}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeCloseRouting(JNIEnv
                                             *env, jclass) {
    frm()->GetRoutingManager().CloseRouting(true /* remove route points */);
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeAddRoutePoint(JNIEnv
                                              *env, jclass,
                                              jstring title,
                                              jstring
                                              subtitle,
                                              jint markType,
                                              jint
                                              intermediateIndex,
                                              jboolean isMyPosition,
                                              jdouble
                                              lat,
                                              jdouble lon
) {
    RouteMarkData data;
    data.
            m_title = jni::ToNativeString(env, title);
    data.
            m_subTitle = jni::ToNativeString(env, subtitle);
    data.
            m_pointType = static_cast<RouteMarkType>(markType);
    data.
            m_intermediateIndex = static_cast<size_t>(intermediateIndex);
    data.
            m_isMyPosition = static_cast<bool>(isMyPosition);
    data.
            m_position = m2::PointD(mercator::FromLatLon(lat, lon));

    frm()->GetRoutingManager().
            AddRoutePoint(std::move(data)
    );
}
JNIEXPORT jstring JNICALL Java_com_ftmap_maps_FTMap_nativeFormatLatLon(JNIEnv *env, jclass, jdouble
lat,
                                                                       jdouble lon,
                                                                       jboolean
                                                                       useDMSFormat) {
    return
            jni::ToJavaString(
                    env, (useDMSFormat
                          ?
                          measurement_utils::FormatLatLonAsDMS(lat, lon,
                                                               2)
                          :
                          measurement_utils::FormatLatLon(lat, lon,
                                                          true /* withSemicolon */,
                                                          6)));
}

JNIEXPORT jobjectArray

JNICALL
Java_com_ftmap_maps_FTMap_nativeGetRoutePoints(JNIEnv *env, jclass) {
    auto const points = frm()->GetRoutingManager().GetRoutePoints();
//com.ftmap.maps
    static jclass const pointClazz = jni::GetGlobalClassRef(env,
                                                            "com/ftmap/maps/RouteMarkData");
    // Java signature : RouteMarkData(String title, String subtitle,
    //                                @RoutePointInfo.RouteMarkType int pointType,
    //                                int intermediateIndex, boolean isVisible, boolean isMyPosition,
    //                                boolean isPassed, double lat, double lon)
    static jmethodID const pointConstructor = jni::GetConstructorID(env, pointClazz,
                                                                    "(Ljava/lang/String;Ljava/lang/String;IIZZZDD)V");
    return jni::ToJavaArray(env, pointClazz, points, [&](JNIEnv *jEnv, RouteMarkData const &data) {
        jni::TScopedLocalRef const title(env, jni::ToJavaString(env, data.m_title));
        jni::TScopedLocalRef const subtitle(env, jni::ToJavaString(env, data.m_subTitle));
        return env->NewObject(pointClazz, pointConstructor,
                              title.get(), subtitle.get(),
                              static_cast<jint>(data.m_pointType),
                              static_cast<jint>(data.m_intermediateIndex),
                              static_cast<jboolean>(data.m_isVisible),
                              static_cast<jboolean>(data.m_isMyPosition),
                              static_cast<jboolean>(data.m_isPassed),
                              mercator::YToLat(data.m_position.y),
                              mercator::XToLon(data.m_position.x));
    });
}

JNIEXPORT jint

JNICALL
Java_com_ftmap_maps_FTMap_nativeOpenRoutePointsTransaction(JNIEnv *env, jclass) {
    return frm()->GetRoutingManager().OpenRoutePointsTransaction();
}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeCancelRoutePointsTransaction(JNIEnv
                                                             *env, jclass,
                                                             jint transactionId
) {
    frm()->GetRoutingManager().
            CancelRoutePointsTransaction(transactionId);

}

JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMap_nativeApplyRoutePointsTransaction(JNIEnv
                                                            *env, jclass,
                                                            jint transactionId
) {
    frm()->GetRoutingManager().
            ApplyRoutePointsTransaction(transactionId);
}
JNIEXPORT void JNICALL
Java_com_ftmap_maps_FTMapFragment_nativeSetRenderingInitializationFinishedListener(
        JNIEnv
        *env,
        jclass clazz, jobject
        listener) {
    if (listener) {
        g_framework->NativeFramework()->SetGraphicsContextInitializationHandler(
                std::bind(&OnRenderingInitializationFinished,
                          jni::make_global_ref(listener)
                ));
    } else {
        g_framework->NativeFramework()->SetGraphicsContextInitializationHandler(nullptr);
    }
}
JNIEXPORT jobjectArray

JNICALL
Java_com_ftmap_maps_FTMap_nativeGenerateNotifications(JNIEnv *env, jclass) {
    ::Framework *fr = frm();
    if (!fr->GetRoutingManager().IsRoutingActive())
        return nullptr;

    std::vector<std::string> notifications;
    fr->GetRoutingManager().GenerateNotifications(notifications);
    if (notifications.empty())
        return nullptr;

    return jni::ToJavaStringArray(env, notifications);
}
JNIEXPORT jint JNICALL
Java_com_ftmap_maps_FTMap_nativeGetDrawScale(JNIEnv *env, jclass) {
    return static_cast<jint>(frm()->GetDrawScale());
}
//JNIEXPORT void JNICALL
//Java_com_ftmap_maps_FTMap_nativeClearApiPoints(JNIEnv *env, jclass clazz) {
//    frm()->GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::API);
//}

}