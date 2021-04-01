package com.ftmap.app;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Message;

import androidx.annotation.NonNull;
import androidx.multidex.MultiDex;
import java.util.HashMap;
import java.util.List;

import com.ftmap.maps.FMap;
import com.ftmap.maps.BuildConfig;
import com.ftmap.maps.R;

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

}
