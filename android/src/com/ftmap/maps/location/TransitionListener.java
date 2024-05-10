package com.ftmap.maps.location;

import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import androidx.annotation.NonNull;

import com.ftmap.maps.FTMap;
import com.ftmap.maps.background.AppBackgroundTracker;

public class TransitionListener implements AppBackgroundTracker.OnTransitionListener
{
  @NonNull
  private final GPSCheck mReceiver = new GPSCheck();
  private boolean mReceiverRegistered;

  @Override
  public void onTransit(boolean foreground)
  {
    if (foreground && !mReceiverRegistered)
    {
      final IntentFilter filter = new IntentFilter();
      filter.addAction(LocationManager.PROVIDERS_CHANGED_ACTION);
      filter.addCategory(Intent.CATEGORY_DEFAULT);

      FTMap.INSTANCE.getApp().registerReceiver(mReceiver, filter);
      mReceiverRegistered = true;
      return;
    }

    if (!foreground && mReceiverRegistered)
    {
      FTMap.INSTANCE.getApp().unregisterReceiver(mReceiver);
      mReceiverRegistered = false;
    }
  }
}
