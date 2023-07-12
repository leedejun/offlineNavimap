package com.ftmap.app;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;


public class MapTestApplication extends Application
{
  public final static String TAG = "MapTestApplication";

  private static MapTestApplication sSelf;
  private boolean mFirstLaunch;

  public MapTestApplication()
  {
    super();
    sSelf = this;
  }

  /**
   * Use the {@link #from(Context)} method instead.
   */
  @Deprecated
  public static MapTestApplication get()
  {
    return sSelf;
  }

  @NonNull
  public static MapTestApplication from(@NonNull Context context)
  {
    return (MapTestApplication) context.getApplicationContext();
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  @Override
  public void onCreate()
  {
    super.onCreate();

  }

  public void setFirstLaunch(boolean isFirstLaunch)
  {
    mFirstLaunch = isFirstLaunch;
  }

  public boolean isFirstLaunch()
  {
    return mFirstLaunch;
  }
  private void initNativeFramework()
  {
//    if (mFrameworkInitialized)
//      return;
//
//    nativeInitFramework();
//
//    MapManager.nativeSubscribe(mStorageCallbacks);
//
//    initNativeStrings();
//    ThemeSwitcher.INSTANCE.initialize(this);
//    SearchEngine.INSTANCE.initialize(null);
//    BookmarkManager.loadBookmarks();
//    TtsPlayer.INSTANCE.initialize(this);
//    ThemeSwitcher.INSTANCE.restart(false);

//    RoutingController.get().initialize(null);
//    TrafficManager.INSTANCE.initialize(null);
//    SubwayManager.from(this).initialize(null);
//    IsolinesManager.from(this).initialize(null);
//    mBackgroundTracker.addListener(this);

  }

  private static Context context;



  public static Context getContext() {
    return context;
  }
}
