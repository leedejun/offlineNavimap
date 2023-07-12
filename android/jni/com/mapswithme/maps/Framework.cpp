#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"
//#include "com/mapswithme/maps/UserMarkHelper.hpp"
#include "com/mapswithme/opengl/androidoglcontextfactory.hpp"
#include "com/mapswithme/platform/Platform.hpp"
#include "com/mapswithme/util/FeatureIdBuilder.hpp"
#include "com/mapswithme/util/NetworkPolicy.hpp"
//#include "com/mapswithme/vulkan/android_vulkan_context_factory.hpp"

#include "map/chart_generator.hpp"
#include "map/everywhere_search_params.hpp"
#include "map/notifications/notification_queue.hpp"
#include "map/user_mark.hpp"
#include "map/purchase.hpp"

#include "web_api/utils.hpp"

#include "storage/storage_defines.hpp"
#include "storage/storage_helpers.hpp"

#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/pointers.hpp"
#include "drape/support_manager.hpp"
#include "drape/visual_scale.hpp"

#include "coding/files_container.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "indexer/feature_altitude.hpp"

#include "routing/following_info.hpp"
#include "routing/speed_camera_manager.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/network_policy.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/sunrise_sunset.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace std::placeholders;
using namespace notifications;

unique_ptr<android::Framework> g_framework;

using namespace storage;
using platform::CountryFile;
using platform::LocalCountryFile;

static_assert(sizeof(int) >= 4, "Size of jint in less than 4 bytes.");

namespace
{
    ::Framework * frm()
    {
      return g_framework->NativeFramework();
    }

//jobject g_placePageActivationListener = nullptr;
    int const kUndefinedTip = -1;

//android::AndroidVulkanContextFactory * CastFactory(drape_ptr<dp::GraphicsContextFactory> const & f)
//{
//  ASSERT(dynamic_cast<android::AndroidVulkanContextFactory *>(f.get()) != nullptr, ());
//  return static_cast<android::AndroidVulkanContextFactory *>(f.get());
//}
}  // namespace

namespace android
{

    enum MultiTouchAction
    {
        MULTITOUCH_UP    =   0x00000001,
        MULTITOUCH_DOWN  =   0x00000002,
        MULTITOUCH_MOVE  =   0x00000003,
        MULTITOUCH_CANCEL =  0x00000004
    };

    Framework::Framework()
            : m_lastCompass(0.0)
            , m_isSurfaceDestroyed(false)
            , m_currentMode(location::PendingPosition)
            , m_isCurrentModeInitialized(false)
            , m_isChoosePositionMode(false)
    {
      m_work.GetPowerManager().Subscribe(this);
    }

    void Framework::OnLocationError(int errorCode)
    {
      m_work.OnLocationError(static_cast<location::TLocationError>(errorCode));
    }

    void Framework::OnLocationUpdated(location::GpsInfo const & info)
    {
      ASSERT(IsDrapeEngineCreated(), ());
      m_work.OnLocationUpdate(info);
    }

    void Framework::OnCompassUpdated(location::CompassInfo const & info, bool forceRedraw)
    {
      static double const COMPASS_THRESHOLD = base::DegToRad(1.0);

      /// @todo Do not emit compass bearing too often.
      /// Need to make more experiments in future.
      if (forceRedraw || fabs(ang::GetShortestDistance(m_lastCompass, info.m_bearing)) >= COMPASS_THRESHOLD)
      {
        m_lastCompass = info.m_bearing;
        m_work.OnCompassUpdate(info);
      }
    }

    void Framework::MyPositionModeChanged(location::EMyPositionMode mode, bool routingActive)
    {
      if (m_myPositionModeSignal)
        m_myPositionModeSignal(mode, routingActive);
    }


    bool Framework::DestroySurfaceOnDetach()
    {
      if (m_vulkanContextFactory)
        return false;
      return true;
    }

    bool Framework::CreateDrapeEngine(JNIEnv * env, jobject jSurface, int densityDpi, bool firstLaunch,
                                      bool launchByDeepLink, int appVersionCode)
    {
      // Vulkan is supported only since Android 8.0, because some Android devices with Android 7.x
      // have fatal driver issue, which can lead to process termination and whole OS destabilization.
      int constexpr kMinSdkVersionForVulkan = 26;
      int const sdkVersion = GetAndroidSdkVersion();
      auto const vulkanForbidden = sdkVersion < kMinSdkVersionForVulkan ||
                                   dp::SupportManager::Instance().IsVulkanForbidden();
//  if (vulkanForbidden)
//    LOG(LWARNING, ("Vulkan API is forbidden on this device."));
//
//  if (m_work.LoadPreferredGraphicsAPI() == dp::ApiVersion::Vulkan && !vulkanForbidden)
//  {
//    m_vulkanContextFactory =
//        make_unique_dp<AndroidVulkanContextFactory>(appVersionCode, sdkVersion);
//    if (!CastFactory(m_vulkanContextFactory)->IsVulkanSupported())
//    {
//      LOG(LWARNING, ("Vulkan API is not supported."));
//      m_vulkanContextFactory.reset();
//    }
//
//    if (m_vulkanContextFactory)
//    {
//      auto f = CastFactory(m_vulkanContextFactory);
//      f->SetSurface(env, jSurface);
//      if (!f->IsValid())
//      {
//        LOG(LWARNING, ("Invalid Vulkan API context."));
//        m_vulkanContextFactory.reset();
//      }
//    }
//  }

      AndroidOGLContextFactory * oglFactory = nullptr;
      if (!m_vulkanContextFactory)
      {
        m_oglContextFactory = make_unique_dp<dp::ThreadSafeFactory>(
                new AndroidOGLContextFactory(env, jSurface));
        oglFactory = m_oglContextFactory->CastFactory<AndroidOGLContextFactory>();
        if (!oglFactory->IsValid())
        {
          LOG(LWARNING, ("Invalid GL context."));
          return false;
        }
      }

      ::Framework::DrapeCreationParams p;
      if (m_vulkanContextFactory)
      {
//    auto f = CastFactory(m_vulkanContextFactory);
//    p.m_apiVersion = dp::ApiVersion::Vulkan;
//    p.m_surfaceWidth = f->GetWidth();
//    p.m_surfaceHeight = f->GetHeight();
      }
      else
      {
        CHECK(oglFactory != nullptr, ());
        p.m_apiVersion = oglFactory->IsSupportedOpenGLES3() ? dp::ApiVersion::OpenGLES3 :
                         dp::ApiVersion::OpenGLES2;
        p.m_surfaceWidth = oglFactory->GetWidth();
        p.m_surfaceHeight = oglFactory->GetHeight();
      }
      p.m_visualScale = static_cast<float>(dp::VisualScale(densityDpi));
      p.m_hasMyPositionState = m_isCurrentModeInitialized;
      p.m_initialMyPositionState = m_currentMode;
      p.m_isChoosePositionMode = m_isChoosePositionMode;
      p.m_hints.m_isFirstLaunch = firstLaunch;
      p.m_hints.m_isLaunchByDeepLink = launchByDeepLink;
//  ASSERT(!m_guiPositions.empty(), ("GUI elements must be set-up before engine is created"));
      p.m_widgetsInitInfo = m_guiPositions;

      m_work.SetMyPositionModeListener(bind(&Framework::MyPositionModeChanged, this, _1, _2));

//  if (m_vulkanContextFactory)
//    m_work.CreateDrapeEngine(make_ref(m_vulkanContextFactory), move(p));
//  else
      m_work.CreateDrapeEngine(make_ref(m_oglContextFactory), move(p));
      m_work.EnterForeground();

      return true;
    }

    bool Framework::IsDrapeEngineCreated()
    {
      return m_work.IsDrapeEngineCreated();
    }

    void Framework::Resize(JNIEnv * env, jobject jSurface, int w, int h)
    {
      if (m_vulkanContextFactory)
      {
//    auto vulkanContextFactory = CastFactory(m_vulkanContextFactory);
//    if (vulkanContextFactory->GetWidth() != w || vulkanContextFactory->GetHeight() != h)
//    {
//      m_vulkanContextFactory->SetPresentAvailable(false);
//      m_work.SetRenderingDisabled(false /* destroySurface */);
//
//      vulkanContextFactory->ChangeSurface(env, jSurface, w, h);
//
//      vulkanContextFactory->SetPresentAvailable(true);
//      m_work.SetRenderingEnabled();
//    }
      }
      else
      {
        m_oglContextFactory->CastFactory<AndroidOGLContextFactory>()->UpdateSurfaceSize(w, h);
      }
      m_work.OnSize(w, h);

      //TODO: remove after correct visible rect calculation.
      frm()->SetVisibleViewport(m2::RectD(0, 0, w, h));
    }

    void Framework::DetachSurface(bool destroySurface)
    {
      LOG(LINFO, ("Detach surface started. destroySurface =", destroySurface));
      if (m_vulkanContextFactory)
      {
//    m_vulkanContextFactory->SetPresentAvailable(false);
      }
      else
      {
        ASSERT(m_oglContextFactory != nullptr, ());
        m_oglContextFactory->SetPresentAvailable(false);
      }

      if (destroySurface)
      {
        LOG(LINFO, ("Destroy surface."));
        m_isSurfaceDestroyed = true;
        m_work.OnDestroySurface();
      }

      if (m_vulkanContextFactory)
      {
        // With Vulkan we don't need to recreate all graphics resources,
        // we have to destroy only resources bound with surface (swapchains,
        // image views, framebuffers and command buffers). All these resources will be
        // destroyed in ResetSurface().
        m_work.SetRenderingDisabled(false /* destroySurface */);

        // Allow pipeline dump only on enter background.
//    CastFactory(m_vulkanContextFactory)->ResetSurface(destroySurface /* allowPipelineDump */);
      }
      else
      {
        m_work.SetRenderingDisabled(destroySurface);
        auto factory = m_oglContextFactory->CastFactory<AndroidOGLContextFactory>();
        factory->ResetSurface();
      }
      LOG(LINFO, ("Detach surface finished."));
    }

    bool Framework::AttachSurface(JNIEnv * env, jobject jSurface)
    {
      LOG(LINFO, ("Attach surface started."));

      int w = 0, h = 0;
      if (m_vulkanContextFactory)
      {
//    auto factory = CastFactory(m_vulkanContextFactory);
//    factory->SetSurface(env, jSurface);
//    if (!factory->IsValid())
//    {
//      LOG(LWARNING, ("Invalid Vulkan API context."));
//      return false;
//    }
//    w = factory->GetWidth();
//    h = factory->GetHeight();
      }
      else
      {
        ASSERT(m_oglContextFactory != nullptr, ());
        auto factory = m_oglContextFactory->CastFactory<AndroidOGLContextFactory>();
        factory->SetSurface(env, jSurface);
        if (!factory->IsValid())
        {
          LOG(LWARNING, ("Invalid GL context."));
          return false;
        }
        w = factory->GetWidth();
        h = factory->GetHeight();
      }

//  ASSERT(!m_guiPositions.empty(), ("GUI elements must be set-up before engine is created"));

      if (m_vulkanContextFactory)
      {
//    m_vulkanContextFactory->SetPresentAvailable(true);
//    m_work.SetRenderingEnabled();
      }
      else
      {
        m_oglContextFactory->SetPresentAvailable(true);
        m_work.SetRenderingEnabled(make_ref(m_oglContextFactory));
      }

      if (m_isSurfaceDestroyed)
      {
        LOG(LINFO, ("Recover surface, viewport size:", w, h));
        bool const recreateContextDependentResources = (m_vulkanContextFactory == nullptr);
        m_work.OnRecoverSurface(w, h, recreateContextDependentResources);
        m_isSurfaceDestroyed = false;
      }

      LOG(LINFO, ("Attach surface finished."));

      return true;
    }

    void Framework::PauseSurfaceRendering()
    {
//  if (m_vulkanContextFactory)
//    m_vulkanContextFactory->SetPresentAvailable(false);
      if (m_oglContextFactory)
        m_oglContextFactory->SetPresentAvailable(false);

      LOG(LINFO, ("Pause surface rendering."));
    }

    void Framework::ResumeSurfaceRendering()
    {
//  if (m_vulkanContextFactory)
//  {
//    if (CastFactory(m_vulkanContextFactory)->IsValid())
//      m_vulkanContextFactory->SetPresentAvailable(true);
//  }
      if (m_oglContextFactory)
      {
        AndroidOGLContextFactory * factory = m_oglContextFactory->CastFactory<AndroidOGLContextFactory>();
        if (factory->IsValid())
          factory->SetPresentAvailable(true);
      }
      LOG(LINFO, ("Resume surface rendering."));
    }

    void Framework::SetMapStyle(MapStyle mapStyle)
    {
      m_work.SetMapStyle(mapStyle);
    }

    void Framework::MarkMapStyle(MapStyle mapStyle)
    {
      // In case of Vulkan rendering we don't recreate geometry and textures data, so
      // we need use SetMapStyle instead of MarkMapStyle in all cases.
//  if (m_vulkanContextFactory)
//    m_work.SetMapStyle(mapStyle);
//  else
      m_work.MarkMapStyle(mapStyle);
    }

    MapStyle Framework::GetMapStyle() const
    {
      return m_work.GetMapStyle();
    }

    void Framework::Save3dMode(bool allow3d, bool allow3dBuildings)
    {
      m_work.Save3dMode(allow3d, allow3dBuildings);
    }

    void Framework::Set3dMode(bool allow3d, bool allow3dBuildings)
    {
      m_work.Allow3dMode(allow3d, allow3dBuildings);
    }

    void Framework::Get3dMode(bool & allow3d, bool & allow3dBuildings)
    {
      m_work.Load3dMode(allow3d, allow3dBuildings);
    }

    void Framework::SetChoosePositionMode(bool isChoosePositionMode, bool isBusiness,
                                          bool hasPosition, m2::PointD const & position)
    {
      m_isChoosePositionMode = isChoosePositionMode;
      m_work.BlockTapEvents(isChoosePositionMode);
      m_work.EnableChoosePositionMode(isChoosePositionMode, isBusiness, hasPosition, position);
    }

    bool Framework::GetChoosePositionMode()
    {
      return m_isChoosePositionMode;
    }

    Storage & Framework::GetStorage()
    {
      return m_work.GetStorage();
    }

    DataSource const & Framework::GetDataSource() { return m_work.GetDataSource(); }

    void Framework::ShowNode(CountryId const & idx, bool zoomToDownloadButton)
    {
      if (zoomToDownloadButton)
      {
        m2::RectD const rect = CalcLimitRect(idx, m_work.GetStorage(), m_work.GetCountryInfoGetter());
        m_work.SetViewportCenter(rect.Center(), 10);
      }
      else
      {
        m_work.ShowNode(idx);
      }
    }

    void Framework::Touch(int action, Finger const & f1, Finger const & f2, uint8_t maskedPointer)
    {
      MultiTouchAction eventType = static_cast<MultiTouchAction>(action);
      df::TouchEvent event;

      switch(eventType)
      {
        case MULTITOUCH_DOWN:
          event.SetTouchType(df::TouchEvent::TOUCH_DOWN);
              break;
        case MULTITOUCH_MOVE:
          event.SetTouchType(df::TouchEvent::TOUCH_MOVE);
              break;
        case MULTITOUCH_UP:
          event.SetTouchType(df::TouchEvent::TOUCH_UP);
              break;
        case MULTITOUCH_CANCEL:
          event.SetTouchType(df::TouchEvent::TOUCH_CANCEL);
              break;
        default:
          return;
      }

      df::Touch touch;
      touch.m_location = m2::PointD(f1.m_x, f1.m_y);
      touch.m_id = f1.m_id;
      event.SetFirstTouch(touch);

      touch.m_location = m2::PointD(f2.m_x, f2.m_y);
      touch.m_id = f2.m_id;
      event.SetSecondTouch(touch);

      event.SetFirstMaskedPointer(maskedPointer);
      m_work.TouchEvent(event);
    }

    m2::PointD Framework::GetViewportCenter() const
    {
      return m_work.GetViewportCenter();
    }

    void Framework::AddString(string const & name, string const & value)
    {
      m_work.AddString(name, value);
    }

    void Framework::Scale(::Framework::EScaleMode mode)
    {
      m_work.Scale(mode, true);
    }

    void Framework::Scale(m2::PointD const & centerPt, int targetZoom, bool animate)
    {
      ref_ptr<df::DrapeEngine> engine = m_work.GetDrapeEngine();
      if (engine)
        engine->SetModelViewCenter(centerPt, targetZoom, animate, false);
    }

    ::Framework * Framework::NativeFramework()
    {
      return &m_work;
    }

    bool Framework::Search(search::EverywhereSearchParams const & params)
    {
      m_searchQuery = params.m_query;
      return m_work.GetSearchAPI().SearchEverywhere(params);
    }

    void Framework::AddLocalMaps()
    {
      m_work.RegisterAllMaps();
    }

    void Framework::RemoveLocalMaps()
    {
      m_work.DeregisterAllMaps();
    }

    void Framework::DeactivatePopup()
    {
//  m_work.DeactivateMapSelection(false);
    }

    string Framework::GetOutdatedCountriesString()
    {
      vector<Country const *> countries;
      class Storage const & storage = GetStorage();
      storage.GetOutdatedCountries(countries);

      string res;
      NodeAttrs attrs;

      for (size_t i = 0; i < countries.size(); ++i)
      {
        storage.GetNodeAttrs(countries[i]->Name(), attrs);

        if (i > 0)
          res += ", ";

        res += attrs.m_nodeLocalName;
      }

      return res;
    }

    void Framework::SetMyPositionModeListener(location::TMyPositionModeChanged const & fn)
    {
      m_myPositionModeSignal = fn;
    }

    location::EMyPositionMode Framework::GetMyPositionMode()
    {
      if (!m_isCurrentModeInitialized)
      {
        if (!settings::Get(settings::kLocationStateMode, m_currentMode))
          m_currentMode = location::NotFollowNoPosition;

        m_isCurrentModeInitialized = true;
      }

      return m_currentMode;
    }

    void Framework::OnMyPositionModeChanged(location::EMyPositionMode mode)
    {
      m_currentMode = mode;
      m_isCurrentModeInitialized = true;
    }

    void Framework::SwitchMyPositionNextMode()
    {
      ASSERT(IsDrapeEngineCreated(), ());
      m_work.SwitchMyPositionNextMode();
    }

    void Framework::SetupWidget(gui::EWidget widget, float x, float y, dp::Anchor anchor)
    {
      m_guiPositions[widget] = gui::Position(m2::PointF(x, y), anchor);
    }

    void Framework::ApplyWidgets()
    {
      gui::TWidgetsLayoutInfo layout;
      for (auto const & widget : m_guiPositions)
        layout[widget.first] = widget.second.m_pixelPivot;

      m_work.SetWidgetLayout(move(layout));
    }

    void Framework::CleanWidgets()
    {
      m_guiPositions.clear();
    }

    void Framework::SetupMeasurementSystem()
    {
      m_work.SetupMeasurementSystem();
    }


    bool Framework::IsAutoRetryDownloadFailed()
    {
      return m_work.GetDownloadingPolicy().IsAutoRetryDownloadFailed();
    }

    bool Framework::IsDownloadOn3gEnabled()
    {
      return m_work.GetDownloadingPolicy().IsCellularDownloadEnabled();
    }

    void Framework::EnableDownloadOn3g()
    {
      m_work.GetDownloadingPolicy().EnableCellularDownload(true);
    }


    int Framework::ToDoAfterUpdate() const
    {
      return (int) m_work.ToDoAfterUpdate();
    }


    void Framework::OnPowerFacilityChanged(power_management::Facility const facility, bool enabled)
    {
      // Dummy
      // TODO: provide information for UI Properties.
    }

    void Framework::OnPowerSchemeChanged(power_management::Scheme const actualScheme)
    {
      // Dummy
      // TODO: provide information for UI Properties.
    }

    FeatureID Framework::BuildFeatureId(JNIEnv * env, jobject featureId)
    {
      static FeatureIdBuilder const builder(env);

      return builder.Build(env, featureId);
    }
}  // namespace android


extern "C"
{

}  // extern "C"
