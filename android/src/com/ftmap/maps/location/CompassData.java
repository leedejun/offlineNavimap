package com.ftmap.maps.location;

import android.app.Activity;

//import com.mapswithme.maps.MwmApplication;
import com.ftmap.util.LocationUtils;
import com.ftmap.maps.FTMap;

public final class CompassData
{
  private double mNorth;

  public void update(double north)
  {
    Activity top = FTMap.INSTANCE.backgroundTracker().getTopActivity();
    if (top == null)
      return;

    int rotation = top.getWindowManager().getDefaultDisplay().getRotation();
    mNorth = LocationUtils.correctCompassAngle(rotation, north);
  }

  public double getNorth()
  {
    return mNorth;
  }
}

