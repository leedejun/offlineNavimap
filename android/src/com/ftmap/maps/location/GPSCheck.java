package com.ftmap.maps.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.ftmap.util.log.Logger;
import com.ftmap.util.log.LoggerFactory;

//import static com.mapswithme.maps.MwmApplication.backgroundTracker;

public class GPSCheck extends BroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);
  private static final String TAG = GPSCheck.class.getSimpleName();
  @Override
  public void onReceive(Context context, Intent intent) {
//    String msg = "onReceive: " + intent + " app in background = "
//                 + !backgroundTracker().isForeground();
//    LOGGER.i(TAG, msg);
    if (true)//MwmApplication.get().arePlatformAndCoreInitialized() && MwmApplication.backgroundTracker().isForeground())
    {
      LocationHelper.INSTANCE.restart();
    }
  }
}
