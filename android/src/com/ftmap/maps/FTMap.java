package com.ftmap.maps;

import android.app.Activity;
import android.app.Application;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import androidx.fragment.app.FragmentActivity;

import com.ftmap.maps.FTMapFragment;
import com.ftmap.maps.background.AppBackgroundTracker;
import com.ftmap.maps.location.LocationHelper;
import com.ftmap.util.StorageUtils;
import com.ftmap.util.Utils;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class FTMap extends FMap implements View.OnTouchListener {
    private Handler mMainLoopHandler;
    private final Object mMainQueueToken = new Object();
    private FTMapFragment mapFragment;
    private AppBackgroundTracker mBackgroundTracker;
    private Application mApp;
    private String mAppId;

    public final static FTMap INSTANCE = new FTMap();

    private FTMap() {
        System.loadLibrary("mapswithme");
    }

//    public class OnMapClickListener {
//        void onMapClick(double x, double y){
//
//        }
//        void onMapPoiClick(double x, double y) {
//        }
//    }
    private class PoiSearchImpl implements  PoiSearch{
        @Override
        public void KeywordSearch() {

        }

        @Override
        public void kindSearch() {

        }

        @Override
        public void AroundSearch() {

        }
    }
    private class DrawImpl implements  Draw{
        @Override
        public void addMarker() {

        }

        @Override
        public void addPolyline() {

        }

        @Override
        public void addCircle() {

        }
    }
    public class ClickListenerImpl implements  ClickListener{
        @Override
        public void OnMapStatusChangeListener() {

        }

        @Override
        public void OnMapClickListener(double x, double y) {

        }


        @Override
        public void OnMapDoubleClickListener() {

        }

        @Override
        public void OnMapLongClickListener() {

        }

        @Override
        public void OnScrollGesturesListener() {

        }
    }
    private class StateSettingsImpl implements  StateSettings{
        @Override
        public void setCenterPoint(Object latLng) {

        }

        @Override
        public Object getCenterPoint() {
            return null;
        }

        @Override
        public void setZoom(double zoom) {

        }

        @Override
        public double getZoom() {
            return 0;
        }

        @Override
        public void setMinZoomPreference(double minZoom) {

        }

        @Override
        public double getMinZoomLevel() {
            return 0;
        }

        @Override
        public void setMaxZoomPreference(double maxZoom) {

        }

        @Override
        public double getMaxZoomLevel() {
            return 0;
        }

        @Override
        public void setTilt(int tilt) {

        }

        @Override
        public int getTilt() {
            return 0;
        }

        @Override
        public void setBearing(int bearing) {

        }

        @Override
        public int getBaering() {
            return 0;
        }

        @Override
        public void setStyle(Object style) {

        }

        @Override
        public Object getStyle() {
            return null;
        }
    }
    private class UiSettingsImpl implements  UiSettings{
        @Override
        public void setCompassEnabled(boolean compassEnabled) {

        }

        @Override
        public void setCompassImage(Drawable compass) {

        }

        @Override
        public Drawable getCompassImage() {
            return null;
        }

        @Override
        public void setCompassMargins(int left, int top, int right, int bottom) {

        }

        @Override
        public void setCompassGravity(int gravity) {

        }

        @Override
        public void setLogoEnabled(boolean enabled) {

        }

        @Override
        public void setRotateGesturesEnabled(boolean rotateGesturesEnabled) {

        }

        @Override
        public boolean isRotateGesturesEnabled() {
            return false;
        }

        @Override
        public void setTiltGesturesEnabled(boolean tiltGesturesEnabled) {

        }

        @Override
        public boolean isTiltGesturesEnabled() {
            return false;
        }

        @Override
        public void setZoomGesturesEnabled(boolean zoomGesturesEnabled) {

        }

        @Override
        public boolean isZoomGesturesEnabled() {
            return false;
        }

        @Override
        public void setZoomControlsEnabled(boolean zoomControlsEnabled) {

        }

        @Override
        public boolean isZoomControlsEnabled() {
            return false;
        }

        @Override
        public void setDoubleTapGesturesEnabled(boolean doubleTapGesturesEnabled) {

        }

        @Override
        public boolean isDoubleTapGesturesEnabled() {
            return false;
        }

        @Override
        public void setScrollGesturesEnabled(boolean scrollGesturesEnabled) {

        }

        @Override
        public boolean isScrollGesturesEnabled() {
            return false;
        }

        @Override
        public float getWidth() {
            return 0;
        }

        @Override
        public float getHeight() {
            return 0;
        }
    }

    private class DrawItemImpl implements DrawItem {
        private String _id;
        private String _color;
        private boolean _visible;
        protected boolean isAdded = false;

        @Override()
        public String id() {
            return this._id;
        }

        @Override()
        public void id(String v) {
            this._id = v;
        }

        @Override()
        public void color(String v) {
            this._color = v;
        }

        @Override()
        public String color() {
            return this._color;
        }

        @Override()
        public void visible(boolean v) {
            this._visible = v;
        }

        @Override()
        public boolean visible() {
            return this._visible;
        }

        @Override()
        public void destroy() {
            FTMap.cmd("removeDrawItem").set("id", this.id()).run();
        }

        @Override()
        public void update() {
        }
    }

    private class PointItemImpl extends DrawItemImpl implements PointItem {
        private Point _pos;
        private double _radius;
        private String _shape = "circle";
        private long _markId = -1;

        @Override()
        public void pos(Point p) {
            this._pos = p;
        }

        @Override()
        public Point pos() {
            return this._pos;
        }

        @Override()
        public void radius(double v) {
            this._radius = v;
        }

        @Override()
        public double radius() {
            return this._radius;
        }

        @Override()
        public void shape(String v) {
            this._shape = v;
        }

        @Override()
        public String shape() {
            return _shape;
        }

        @Override()
        public void update() {
            if (_markId >= 0) {
                this._markId = (long) FTMap.cmd("updateMarkItem")
                        .set("type", "point")
                        .set("markId", this._markId)
                        .set("color", this.color())
                        .set("radius", this.radius())
                        .set("x", this._pos.x)
                        .set("y", this._pos.y)
                        .run().get("result");
            } else {
                this._markId = (long) FTMap.cmd("addDrawItem")
                        .set("type", "point")
                        .set("id", this.id())
                        .set("color", this.color())
                        .set("radius", this.radius())
                        .set("x", this._pos.x)
                        .set("y", this._pos.y)
                        .run().get("result");
            }
        }

        @Override()
        public void destroy() {
            if (_markId >= 0) {
                FTMap.cmd("removeMarkItem").set("markId", this._markId).run();
                this._markId = -1;
            }
        }
    }

    private class LineItemImpl extends DrawItemImpl implements LineItem {
        private JSONArray _points;
        private double _width;

        @Override()
        public void points(JSONArray points) {
            this._points = points;
        }

        @Override()
        public JSONArray points() {
            return this._points;
        }

        @Override()
        public void width(double v) {
            this._width = v;
        }

        @Override()
        public double width() {
            return this._width;
        }

        @Override()
        public void update() {
            if (this.isAdded) {
                FTMap.cmd("updateDrawItem").run();
            } else {
                //this.isAdded = true;
                FTMap.cmd("addDrawItem")
                        .set("type", "line")
                        .set("id", this.id())
                        .set("color", this.color())
                        .set("width", this.width())
                        .set("points", this.points())
                        .run();
            }
        }
    }

    private class AreaItemImpl extends DrawItemImpl implements AreaItem {
        private List<Point> _points;

        @Override()
        public void points(List<Point> points) {
            this._points = points;
        }

        public List<Point> points() {
            return this._points;
        }
    }

    public ClickListener createClickListener() {

        return new ClickListenerImpl();
    }

    @Override
    public void setClickListener(ClickListener clickListener) {

    }

    public DrawItem createDrawItem(DrawItemType type) {
        if (type == DrawItemType.AREA)
            return new AreaItemImpl();
        if (type == DrawItemType.LINE)
            return new LineItemImpl();
        if (type == DrawItemType.POINT)
            return new PointItemImpl();
        return null;
    }

    public Application getApp() {
        return this.mApp;
    }

    public String appId() {
        return this.mAppId;
    }

    public AppBackgroundTracker backgroundTracker() {
        return mBackgroundTracker;
    }


    private boolean initEngine(Application app, String appId) {
        this.mApp = app;
        this.mAppId = appId;

        mBackgroundTracker = new AppBackgroundTracker();
        mMainLoopHandler = new Handler(app.getMainLooper());
        final String settingsPath = StorageUtils.getSettingsPath();
        final String filesPath = StorageUtils.getFilesPath(app);
        final String tempPath = StorageUtils.getTempPath(app);
        if (!(StorageUtils.createDirectory(settingsPath) &&
                StorageUtils.createDirectory(filesPath) &&
                StorageUtils.createDirectory(tempPath)))
            return false;
        Boolean result = (Boolean) cmd("initPlatform")
                .set("thisInstance", this)
                .set("apkPath", StorageUtils.getApkPath(app))
                .set("storagePath", settingsPath)
                .set("filesPath", filesPath)
                .set("tempPath", tempPath)
                .set("obbPath", "")
                .set("flavor", "")
                .set("buildType", "")
                .set("isTablet", false)
                .run()
                .get("result");
        if (result) {
            LocationHelper.INSTANCE.initialize(null);
            LocationHelper.INSTANCE.onEnteredIntoFirstRun();
            if (!LocationHelper.INSTANCE.isActive())
                LocationHelper.INSTANCE.start();
        }
        return result.booleanValue();
    }

    @Override
    public void nativeSetupWidget(int widget, float x, float y, int anchor, SearchResultsCallback callback) {
        cmd("nativeSetupWidget")
                .set("query", widget)
                .set("lat", 116.309)
                .set("lon", 43.7)
                .setAsyncCallback((Object obj) -> {
                    JSONArray array = (JSONArray) obj;
                    callback.op(array);
                })
                .run();
    }

    private void initMap(FragmentActivity ctx, int resId) {
        mapFragment = (FTMapFragment) ctx.getSupportFragmentManager().findFragmentByTag(FTMapFragment.class.getName());
        if (mapFragment == null) {
            Bundle args = new Bundle();
            args.putBoolean(FTMapFragment.ARG_LAUNCH_BY_DEEP_LINK, false);
            mapFragment = (FTMapFragment) FTMapFragment.instantiate(ctx, FTMapFragment.class.getName(), args);
            ctx.getSupportFragmentManager()
                    .beginTransaction()
                    .replace(resId, mapFragment, FTMapFragment.class.getName())
                    .commit();

        }
        View container = ctx.findViewById(resId);
        if (container != null) {
            container.setOnTouchListener(this);
        }
    }

    @Override
    public boolean init(Application app, FragmentActivity ctx, int mapResId, String appId) {
        if (!this.initEngine(app, appId))
            return false;
        this.initMap(ctx, mapResId);
        return true;
    }

    public boolean arePlatformAndCoreInitialized() {
        return true;
    }

    @Override
    public boolean onTouch(View view, MotionEvent event) {
        return mapFragment != null && mapFragment.onTouch(view, event);
    }

    @SuppressWarnings("unused")
    void forwardToMainThread(final long taskPointer) {
        Message m = Message.obtain(mMainLoopHandler, new Runnable() {
            @Override
            public void run() {
                cmd("processTask").set("taskPointer", taskPointer).run();
            }
        });
        m.obj = mMainQueueToken;
        mMainLoopHandler.sendMessage(m);
    }

    public static class Command {
        private Map<String, Object> _data = new HashMap<String, Object>();

        private interface AsyncCallback {
            public void op(Object obj);
        }

        private AsyncCallback _asyncCallback = null;

        public Command() {
        }

        public Command setAsyncCallback(AsyncCallback value) {
            _asyncCallback = value;
            return this;
        }

        public Object get(String key) {
            return _data.get(key);
        }

        public Command set(String key, Object data) {
            _data.put(key, data);
            return this;
        }

        public Command run() {
            runCmd(this);
            return this;
        }

        public void asyncResult(Object obj) {
            if (_asyncCallback != null) {
                _asyncCallback.op(obj);
            }
        }
    }

    public Command buildCmd(String cmd) {
        Command msg = new Command();
        msg.set("_command", cmd);
        return msg;
    }

    public void nativeRes(Command msg) {

    }

    public static Command cmd(String p) {
        Command res = new Command();
        res.set("_command", p);
        return res;
    }

    public static Command runCmd(Command msg) {
        return nativeReq(msg);
    }

    private static native Command nativeReq(Command msg);


    /**
     * 普通poi检索
     *
     * @param query
     * @param callback
     */
    @Override
    public void search(String query, SearchResultsCallback callback) {
        cmd("Search")
                .set("query", query)
                .set("lat", 116.309)
                .set("lon", 43.7)
                .setAsyncCallback((Object obj) -> {
                    JSONArray array = (JSONArray) obj;
                    callback.op(array);
                })
                .run();
    }

    @Override
    public void search(String keywords, double centerX, double centerY, double radius, SearchResultsCallback callback) {

        cmd("aroundSearch")
                .set("query", keywords)
                .set("lat", centerX)
                .set("lon", centerY)
                .setAsyncCallback((Object obj) -> {
                    JSONArray array = (JSONArray) obj;
                    callback.op(array);
                })
                .run();

    }

    @Override
    public void PtoG(double screenX, double screenY, MapUtilsResultsCallback callback) {
        cmd("PtoG")
                .set("screenX", screenX)
                .set("screenY", screenY)
                .setAsyncCallback((Object obj) -> {
                    JSONObject objData = (JSONObject) obj;
                    callback.op(objData);
                })
                .run();
    }

    @Override
    public void ToLatLon(double mercatorX, double mercatorY, MapUtilsResultsCallback callback) {
        cmd("ToLatLon")
                .set("mercatorX", mercatorX)
                .set("mercatorY", mercatorY)
                .setAsyncCallback((Object obj) -> {
                    JSONObject objData = (JSONObject) obj;
                    callback.op(objData);
                })
                .run();
    }

    @Override
    public void FromLatLon(double lat, double lon, MapUtilsResultsCallback callback) {
        cmd("FromLatLon")
                .set("lat", lat)
                .set("lon", lon)
                .setAsyncCallback((Object obj) -> {
                    JSONObject objData = (JSONObject) obj;
                    callback.op(objData);
                })
                .run();
    }

    @Override
    public void GetFeatureID(double mercatorX, double mercatorY, MapUtilsResultsCallback callback) {
        cmd("GetFeatureID")
                .set("mercatorX", mercatorX)
                .set("mercatorY", mercatorY)
                .setAsyncCallback((Object obj) -> {
                    JSONObject objData = (JSONObject) obj;
                    callback.op(objData);
                })
                .run();
    }

    @Override
    public void GetMapObject(double mercatorX, double mercatorY, MapUtilsResultsCallback callback) {
        cmd("GetMapObject")
                .set("mercatorX", mercatorX)
                .set("mercatorY", mercatorY)
                .setAsyncCallback((Object obj) -> {
                    JSONObject objData = (JSONObject) obj;
                    callback.op(objData);
                })
                .run();
    }

    @Override
    public void OnMapClickListener(ClickListenerCallback callback) {

        FTMapFragment.setOnMapClickListener(new FTMapFragment.OnMapClickListener() {
            @Override
            public void Callback(JSONObject dataStr) {
                callback.op(dataStr);

            }
        });
//        mapFragment
//        mapFragment != null && mapFragment.onTouch(view, event);
//       in
//        cmd("ClickListener")
//                .setAsyncCallback((Object obj) -> {
//                    JSONObject objData = (JSONObject) obj;
//                    callback.op(objData);
//                })
//                .run();

    }

    @Override
    public void zoom(boolean isIn) {
        if (isIn)
            FTMap.cmd("scalePlus").run();
        else
            FTMap.cmd("scaleMinus").run();
    }

    @Override
    public void setViewCenter(double lat, double lon, int zoom) {
        FTMap.cmd("setViewCenter").set("lat", lat).set("lon", lon).set("zoom", zoom).run();
    }

    /**
     * 将点集合全部显示到视图中间
     *
     * @param points
     */
    @Override
    public void centerPoints(JSONArray points) {
        FTMap.cmd("centerPoints").set("points", points).run();
    }

    public void centerPoints(List<Point> points) {
        try {
            JSONArray array = new JSONArray();
            for (int i = 0; i < points.size(); i++) {
                Point p = points.get(i);
                array.put(p.x);
                array.put(p.y);
            }
            FTMap.cmd("centerPoints").set("points", array).run();
        } catch (Exception e) {

        }
    }

    @Override
    public void removeRoute() {
        FTMap.cmd("removeRoute").run();
    }

    @Override
    public void followRoute() {
        FTMap.cmd("followRoute").run();
    }

    @Override
    public void CloseRouting() {
        FTMap.cmd("CloseRouting").run();
    }


    /**
     * 路径规划
     *
     * @param points
     * @param type
     * @param callback
     */
    @Override
    public void routing(JSONArray points, String type, RoutingResultsCallback callback) {
        FTMap.cmd("route").set("points", points).set("type", type).setAsyncCallback((Object obj) -> {
            JSONObject array = (JSONObject) obj;
            callback.op(array);
        }).run();
    }

    /**
     * 改变地图风格
     *
     * @param style
     */
    public void setMapStyle(String style) {
        FTMap.cmd("setMapStyle").set("style", style).run();
    }

    //
//    public void zoom(String query){
//    }
//
//    public void routing(String query){
//    }
//
//    public void navigate(String query){
//    }
//
//    public void showFeature(String query){
//    }
//
//    public void addShape(String query){
//    }
}
