package com.ftmap.maps.api;


import com.ftmap.maps.Framework;
import com.ftmap.maps.api.RoutePoint;

/**
 * Represents Framework::ParsedRoutingData from core.
 */
public class ParsedRoutingData
{
  public final RoutePoint[] mPoints;
  @Framework.RouterType
  public final int mRouterType;

  public ParsedRoutingData(RoutePoint[] points, int routerType) {
    this.mPoints = points;
    this.mRouterType = routerType;
  }
}
