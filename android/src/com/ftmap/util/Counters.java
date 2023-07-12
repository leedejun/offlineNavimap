package com.ftmap.util;

import android.content.Context;
import android.text.TextUtils;
import android.text.format.DateUtils;

import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.preference.PreferenceManager;

import com.ftmap.app.MapTestApplication;
import com.ftmap.maps.BuildConfig;
import com.ftmap.maps.R;


public final class Counters
{
  static final String KEY_APP_LAUNCH_NUMBER = "LaunchNumber";
  static final String KEY_APP_FIRST_INSTALL_VERSION = "FirstInstallVersion";
  static final String KEY_APP_FIRST_INSTALL_FLAVOR = "FirstInstallFlavor";
  static final String KEY_APP_LAST_SESSION_TIMESTAMP = "LastSessionTimestamp";
  static final String KEY_APP_SESSION_NUMBER = "SessionNumber";
  static final String KEY_MISC_FIRST_START_DIALOG_SEEN = "FirstStartDialogSeen";
  static final String KEY_MISC_NEWS_LAST_VERSION = "WhatsNewShownVersion";
  static final String KEY_LIKES_LAST_RATED_SESSION = "LastRatedSession";

  private static final String KEY_LIKES_RATED_DIALOG = "RatedDialog";

  private Counters() {}

  public static void initCounters(@NonNull Context context)
  {
    PreferenceManager.setDefaultValues(context, R.xml.prefs_main, false);
//    updateLaunchCounter(context);
  }

//  public static int getFirstInstallVersion(@NonNull Context context)
//  {
//    return MapTestApplication.prefs(context).getInt(KEY_APP_FIRST_INSTALL_VERSION, 0);
//  }
//
//  public static boolean isFirstLaunch(@NonNull Context context)
//  {
//    return !MapTestApplication.prefs(context).getBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, false);
//  }
//
//  public static void setFirstStartDialogSeen(@NonNull Context context)
//  {
//    MapTestApplication.prefs(context)
//        .edit()
//        .putBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, true)
//        .apply();
//  }
//
//  public static void resetAppSessionCounters(@NonNull Context context)
//  {
//    MapTestApplication.prefs(context).edit()
//                  .putInt(KEY_APP_LAUNCH_NUMBER, 0)
//                  .putInt(KEY_APP_SESSION_NUMBER, 0)
//                  .putLong(KEY_APP_LAST_SESSION_TIMESTAMP, 0L)
//                  .putInt(KEY_LIKES_LAST_RATED_SESSION, 0)
//                  .apply();
//    incrementSessionNumber(context);
//  }
//
//  public static boolean isSessionRated(@NonNull Context context, int session)
//  {
//    return (MapTestApplication.prefs(context).getInt(KEY_LIKES_LAST_RATED_SESSION,
//                                                 0) >= session);
//  }
//
//  public static void setRatedSession(@NonNull Context context, int session)
//  {
//    MapTestApplication.prefs(context).edit()
//                  .putInt(KEY_LIKES_LAST_RATED_SESSION, session)
//                  .apply();
//  }
//
//  /**
//   * Session = single day, when app was started any number of times.
//   */
//  public static int getSessionCount(@NonNull Context context)
//  {
//    return MapTestApplication.prefs(context).getInt(KEY_APP_SESSION_NUMBER, 0);
//  }
//
//  public static boolean isRatingApplied(@NonNull Context context,
//                                        Class<? extends DialogFragment> dialogFragmentClass)
//  {
//    return MapTestApplication.prefs(context)
//                         .getBoolean(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName(),
//                                     false);
//  }
//
//  public static void setRatingApplied(@NonNull Context context,
//                                      Class<? extends DialogFragment> dialogFragmentClass)
//  {
//    MapTestApplication.prefs(context).edit()
//                  .putBoolean(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName(), true)
//                  .apply();
//  }
//
//  public static String getInstallFlavor(@NonNull Context context)
//  {
//    return MapTestApplication.prefs(context).getString(KEY_APP_FIRST_INSTALL_FLAVOR, "");
//  }
//
//  private static void updateLaunchCounter(@NonNull Context context)
//  {
//    if (incrementLaunchNumber(context) == 0)
//    {
//      if (getFirstInstallVersion(context) == 0)
//      {
//        MapTestApplication.prefs(context)
//                      .edit()
//                      .putInt(KEY_APP_FIRST_INSTALL_VERSION, BuildConfig.VERSION_CODE)
//                      .apply();
//      }
//
//      updateInstallFlavor(context);
//    }
//
//    incrementSessionNumber(context);
//  }
//
//  private static int incrementLaunchNumber(@NonNull Context context)
//  {
//    return increment(context, KEY_APP_LAUNCH_NUMBER);
//  }
//
//  private static void updateInstallFlavor(@NonNull Context context)
//  {
//    String installedFlavor = getInstallFlavor(context);
//    if (TextUtils.isEmpty(installedFlavor))
//    {
//      MapTestApplication.prefs(context).edit()
//                    .putString(KEY_APP_FIRST_INSTALL_FLAVOR, BuildConfig.FLAVOR)
//                    .apply();
//    }
//  }
//
//  private static void incrementSessionNumber(@NonNull Context context)
//  {
//    long lastSessionTimestamp = MapTestApplication.prefs(context)
//                                              .getLong(KEY_APP_LAST_SESSION_TIMESTAMP, 0);
//    if (DateUtils.isToday(lastSessionTimestamp))
//      return;
//
//    MapTestApplication.prefs(context).edit()
//                  .putLong(KEY_APP_LAST_SESSION_TIMESTAMP, System.currentTimeMillis())
//                  .apply();
//    increment(context, KEY_APP_SESSION_NUMBER);
//  }
//
//  private static int increment(@NonNull Context context, @NonNull String key)
//  {
//    int value = MapTestApplication.prefs(context).getInt(key, 0);
//    MapTestApplication.prefs(context).edit()
//                  .putInt(key, ++value)
//                  .apply();
//    return value;
//  }
}
