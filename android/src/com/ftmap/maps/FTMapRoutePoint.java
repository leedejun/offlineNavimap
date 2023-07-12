package com.ftmap.maps;


import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

/**
 * 路径坐标点
 * @author yanchuanbin
 * @version V1.0
 * @date 2018/10/24 9:36
 * @Description: TODO
 */
public class FTMapRoutePoint {


    private static FTMapPoint StartPoint;
    private static FTMapPoint EndPoint;
    private static ArrayList<FTMapPoint> WayPoint =new ArrayList<>();

    public static JSONArray toJsonArr() {
        JSONArray points = new JSONArray();
        try {
            JSONObject sPoint = new JSONObject();
            sPoint.put("lon", StartPoint.lon);
            sPoint.put("lat", StartPoint.lat);
            sPoint.put("name", StartPoint.name);
            sPoint.put("isMyPosition", StartPoint.isMyPosition);
            points.put(sPoint);
            for (int i = 0; i < WayPoint.size(); i++) {
                JSONObject wPoint = new JSONObject();
                FTMapPoint ftMapPoint = WayPoint.get(i);
                wPoint.put("lon", ftMapPoint.lon);
                wPoint.put("lat", ftMapPoint.lat);
                wPoint.put("name", ftMapPoint.name);
                wPoint.put("isMyPosition", ftMapPoint.isMyPosition);
                points.put(wPoint);
            }
            JSONObject ePoint = new JSONObject();
            ePoint.put("lon", EndPoint.lon);
            ePoint.put("lat", EndPoint.lat);
            ePoint.put("name", EndPoint.name);
            ePoint.put("isMyPosition", EndPoint.isMyPosition);
            points.put(ePoint);


        } catch (JSONException e) {
            e.printStackTrace();
        }

        return points;
    }

    public FTMapPoint getStartPoint() {
        return StartPoint;
    }

    public void setStartPoint(FTMapPoint startPoint) {
        StartPoint = startPoint;
    }

    public FTMapPoint getEndPoint() {
        return EndPoint;
    }

    public void setEndPoint(FTMapPoint EndPoint) {
        this.EndPoint = EndPoint;
    }

    public ArrayList<FTMapPoint> getWayPoint() {
        return WayPoint;
    }

    public void setWayPoint(ArrayList<FTMapPoint> wayPoint) {
        WayPoint = wayPoint;
    }

    public void removeAllWayPoint() {
        WayPoint.clear();
    }

    public void addWayPoint(FTMapPoint ftMapPointWay) {
        WayPoint.add(ftMapPointWay);
    }

    public boolean isPlanStatus() {
        boolean isPlanStatus = false;

        if (StartPoint!=null&&EndPoint!=null){
            isPlanStatus =true;
        }
        return isPlanStatus;
    }

    public static class FTMapPoint {
        private Double lat;
        private Double lon;
        private boolean isMyPosition;
        private String name;

        public Double getLat() {
            return lat;
        }

        public void setLat(Double lat) {
            this.lat = lat;
        }

        public Double getLon() {
            return lon;
        }

        public void setLon(Double lon) {
            this.lon = lon;
        }

        public boolean isMyPosition() {
            return isMyPosition;
        }

        public void setMyPosition(boolean myPosition) {
            isMyPosition = myPosition;
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }
    }
}
