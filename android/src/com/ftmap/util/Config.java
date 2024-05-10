package com.ftmap.util;



import static com.ftmap.util.Counters.KEY_APP_FIRST_INSTALL_FLAVOR;
import static com.ftmap.util.Counters.KEY_APP_FIRST_INSTALL_VERSION;
import static com.ftmap.util.Counters.KEY_APP_LAST_SESSION_TIMESTAMP;
import static com.ftmap.util.Counters.KEY_APP_LAUNCH_NUMBER;
import static com.ftmap.util.Counters.KEY_APP_SESSION_NUMBER;
import static com.ftmap.util.Counters.KEY_LIKES_LAST_RATED_SESSION;
import static com.ftmap.util.Counters.KEY_MISC_FIRST_START_DIALOG_SEEN;
import static com.ftmap.util.Counters.KEY_MISC_NEWS_LAST_VERSION;

import android.content.Context;

import androidx.annotation.NonNull;

import com.ftmap.app.MapTestApplication;
import com.ftmap.maps.BuildConfig;
import com.ftmap.maps.NetworkPolicy;
import com.ftmap.maps.R;


public final class Config
{
  private static final String KEY_APP_STORAGE = "StoragePath";

  private static final String KEY_TTS_ENABLED = "TtsEnabled";
  private static final String KEY_TTS_LANGUAGE = "TtsLanguage";

  private static final String KEY_DOWNLOADER_AUTO = "AutoDownloadEnabled";
  private static final String KEY_PREF_ZOOM_BUTTONS = "ZoomButtonsEnabled";
  static final String KEY_PREF_STATISTICS = "StatisticsEnabled";
  static final String KEY_PREF_CRASHLYTICS = "CrashlyticsEnabled";
  private static final String KEY_PREF_USE_GS = "UseGoogleServices";

  private static final String KEY_MISC_DISCLAIMER_ACCEPTED = "IsDisclaimerApproved";
  private static final String KEY_MISC_LOCATION_REQUESTED = "LocationRequested";
  private static final String KEY_MISC_UI_THEME = "UiTheme";
  private static final String KEY_MISC_UI_THEME_SETTINGS = "UiThemeSettings";
  private static final String KEY_MISC_USE_MOBILE_DATA = "UseMobileData";
  private static final String KEY_MISC_USE_MOBILE_DATA_TIMESTAMP = "UseMobileDataTimestamp";
  private static final String KEY_MISC_USE_MOBILE_DATA_ROAMING = "UseMobileDataRoaming";
  private static final String KEY_MISC_AD_FORBIDDEN = "AdForbidden";
  private static final String KEY_MISC_ENABLE_SCREEN_SLEEP = "EnableScreenSleep";
  private static final String KEY_MISC_SHOW_ON_LOCK_SCREEN = "ShowOnLockScreen";
  private static final String KEY_MISC_AGPS_TIMESTAMP = "AGPSTimestamp";
  private static final String KEY_DONATE_URL = "DonateUrl";

  private Config() {}

  private static int getInt(String key)
  {
    return getInt(key, 0);
  }

  private static int getInt(String key, int def)
  {
    return nativeGetInt(key, def);
  }

  private static long getLong(String key)
  {
    return getLong(key, 0L);
  }

  private static long getLong(String key, long def)
  {
    return nativeGetLong(key, def);
  }

  @NonNull
  private static String getString(String key)
  {
    return getString(key, "");
  }

  @NonNull
  private static String getString(String key, String def)
  {
    return nativeGetString(key, def);
  }

  private static boolean getBool(String key)
  {
    return getBool(key, false);
  }

  private static boolean getBool(String key, boolean def)
  {
    return nativeGetBoolean(key, def);
  }

  private static void setInt(String key, int value)
  {
    nativeSetInt(key, value);
  }

  private static void setLong(String key, long value)
  {
    nativeSetLong(key, value);
  }

  private static void setString(String key, String value)
  {
    nativeSetString(key, value);
  }

  private static void setBool(String key)
  {
    setBool(key, true);
  }

  private static void setBool(String key, boolean value)
  {
    nativeSetBoolean(key, value);
  }

  public static void migrateCountersToSharedPrefs(@NonNull Context context)
  {
    int version = getInt(KEY_APP_FIRST_INSTALL_VERSION, BuildConfig.VERSION_CODE);
//    MapTestApplication.prefs(context)
//            .edit()
//            .putInt(KEY_APP_LAUNCH_NUMBER, getInt(KEY_APP_LAUNCH_NUMBER))
//            .putInt(KEY_APP_FIRST_INSTALL_VERSION, version)
//            .putString(KEY_APP_FIRST_INSTALL_FLAVOR, getString(KEY_APP_FIRST_INSTALL_FLAVOR))
//            .putLong(KEY_APP_LAST_SESSION_TIMESTAMP, getLong(KEY_APP_LAST_SESSION_TIMESTAMP))
//            .putInt(KEY_APP_SESSION_NUMBER, getInt(KEY_APP_SESSION_NUMBER))
//            .putBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN,
//                    getBool(KEY_MISC_FIRST_START_DIALOG_SEEN))
//            .putInt(KEY_MISC_NEWS_LAST_VERSION, getInt(KEY_MISC_NEWS_LAST_VERSION))
//            .putInt(KEY_LIKES_LAST_RATED_SESSION, getInt(KEY_LIKES_LAST_RATED_SESSION))
//            .putBoolean(KEY_MISC_ENABLE_SCREEN_SLEEP,
//                    getBool(KEY_MISC_ENABLE_SCREEN_SLEEP))
//            .apply();
  }

  public static String getStoragePath()
  {
    return getString(KEY_APP_STORAGE);
  }

  public static void setStoragePath(String path)
  {
    setString(KEY_APP_STORAGE, path);
  }

  public static boolean isTtsEnabled()
  {
//    return true;
   return getBool(KEY_TTS_ENABLED, true);
  }

  public static void setTtsEnabled(boolean enabled)
  {
    setBool(KEY_TTS_ENABLED, enabled);
  }

  public static String getTtsLanguage()
  {
    return getString(KEY_TTS_LANGUAGE);
  }

  public static void setTtsLanguage(String language)
  {
    setString(KEY_TTS_LANGUAGE, language);
  }

  public static boolean isAutodownloadEnabled()
  {
    return getBool(KEY_DOWNLOADER_AUTO, true);
  }

  public static void setAutodownloadEnabled(boolean enabled)
  {
    setBool(KEY_DOWNLOADER_AUTO, enabled);
  }

  public static boolean showZoomButtons()
  {
    return getBool(KEY_PREF_ZOOM_BUTTONS, true);
  }

  public static void setShowZoomButtons(boolean show)
  {
    setBool(KEY_PREF_ZOOM_BUTTONS, show);
  }

  public static void setStatisticsEnabled(boolean enabled)
  {
    setBool(KEY_PREF_STATISTICS, enabled);
  }

  public static boolean isScreenSleepEnabled()
  {
    return getBool(KEY_MISC_ENABLE_SCREEN_SLEEP, false);
  }

  public static void setScreenSleepEnabled(boolean enabled)
  {
    setBool(KEY_MISC_ENABLE_SCREEN_SLEEP, enabled);
  }

  public static boolean isShowOnLockScreenEnabled()
  {
    return getBool(KEY_MISC_SHOW_ON_LOCK_SCREEN, true);
  }

  public static void setShowOnLockScreenEnabled(boolean enabled)
  {
    setBool(KEY_MISC_SHOW_ON_LOCK_SCREEN, enabled);
  }

  public static boolean useGoogleServices()
  {
    return getBool(KEY_PREF_USE_GS, true);
  }

  public static void setUseGoogleService(boolean use)
  {
    setBool(KEY_PREF_USE_GS, use);
  }

  public static boolean isRoutingDisclaimerAccepted()
  {
    return getBool(KEY_MISC_DISCLAIMER_ACCEPTED);
  }

  public static void acceptRoutingDisclaimer()
  {
    setBool(KEY_MISC_DISCLAIMER_ACCEPTED);
  }

  public static boolean isLocationRequested()
  {
    return getBool(KEY_MISC_LOCATION_REQUESTED);
  }

  public static void setLocationRequested()
  {
    setBool(KEY_MISC_LOCATION_REQUESTED);
  }

  @NonNull
  public static String getCurrentUiTheme(@NonNull Context context)
  {
    String defaultTheme = MapTestApplication.from(context).getString(R.string.theme_default);
    String res = getString(KEY_MISC_UI_THEME, defaultTheme);

    if (ThemeUtils.isValidTheme(context, res))
      return res;

    return defaultTheme;
  }

  static void setCurrentUiTheme(@NonNull Context context, @NonNull String theme)
  {
    if (getCurrentUiTheme(context).equals(theme))
      return;

    setString(KEY_MISC_UI_THEME, theme);
  }

  @NonNull
  public static String getUiThemeSettings(@NonNull Context context)
  {
    String autoTheme = MapTestApplication.from(context).getString(R.string.theme_auto);
    String res = getString(KEY_MISC_UI_THEME_SETTINGS, autoTheme);
    if (ThemeUtils.isValidTheme(context, res) || ThemeUtils.isAutoTheme(context, res))
      return res;

    return autoTheme;
  }

  public static boolean setUiThemeSettings(@NonNull Context context, String theme)
  {
    if (getUiThemeSettings(context).equals(theme))
      return false;

    setString(KEY_MISC_UI_THEME_SETTINGS, theme);
    return true;
  }

  public static boolean isLargeFontsSize()
  {
    return nativeGetLargeFontsSize();
  }

  public static void setLargeFontsSize(boolean value)
  {
    nativeSetLargeFontsSize(value);
  }

  @NonNull
  public static NetworkPolicy.Type getUseMobileDataSettings()
  {
    int value = getInt(KEY_MISC_USE_MOBILE_DATA, NetworkPolicy.NONE);

    if (value < 0 || value >= NetworkPolicy.Type.values().length)
      return NetworkPolicy.Type.ASK;

    return NetworkPolicy.Type.values()[value];
  }

  public static void setUseMobileDataSettings(@NonNull NetworkPolicy.Type value)
  {
    setInt(KEY_MISC_USE_MOBILE_DATA, value.ordinal());
    setBool(KEY_MISC_USE_MOBILE_DATA_ROAMING, ConnectionState.INSTANCE.isInRoaming());
  }

  public static void setMobileDataTimeStamp(long timestamp)
  {
    setLong(KEY_MISC_USE_MOBILE_DATA_TIMESTAMP, timestamp);
  }

  public static long getMobileDataTimeStamp()
  {
    return getLong(KEY_MISC_USE_MOBILE_DATA_TIMESTAMP, 0L);
  }

  public static boolean getMobileDataRoaming()
  {
    return getBool(KEY_MISC_USE_MOBILE_DATA_ROAMING, false);
  }

  public static void setAgpsTimestamp(long timestamp)
  {
    setLong(KEY_MISC_AGPS_TIMESTAMP, timestamp);
  }

  public static long getAgpsTimestamp()
  {
    return getLong(KEY_MISC_AGPS_TIMESTAMP, 0L);
  }

  public static boolean isTransliteration()
  {
    return nativeGetTransliteration();
  }

  public static void setTransliteration(boolean value)
  {
    nativeSetTransliteration(value);
  }

  @NonNull
  public static String getDonateUrl()
  {
    return getString(KEY_DONATE_URL);
  }
  public static boolean isNY()
  {
    return getBool("NY");
  }

  public static native boolean nativeGetBoolean(String name, boolean defaultValue);
  public static native void nativeSetBoolean(String name, boolean value);
  public static native int nativeGetInt(String name, int defaultValue);
  public static native void nativeSetInt(String name, int value);
  public static native long nativeGetLong(String name, long defaultValue);
  public static native void nativeSetLong(String name, long value);
  public static native double nativeGetDouble(String name, double defaultValue);
  public static native void nativeSetDouble(String name, double value);
  public static native String nativeGetString(String name, String defaultValue);
  public static native void nativeSetString(String name, String value);
  public static native boolean nativeGetLargeFontsSize();
  public static native void nativeSetLargeFontsSize(boolean value);
  public static native boolean nativeGetTransliteration();
  public static native void nativeSetTransliteration(boolean value);
}