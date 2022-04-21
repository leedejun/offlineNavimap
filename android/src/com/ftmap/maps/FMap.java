package com.ftmap.maps;

import android.app.Activity;
import android.app.Application;
import android.graphics.Point;
import android.graphics.drawable.Drawable;


import androidx.fragment.app.FragmentActivity;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.List;

public abstract class FMap {
    public final static FMap INSTANCE = FTMap.INSTANCE;
    private UiSettings uiSettings;
    private StateSettings stateSettings;
    private ClickListener clickListener;
    private Draw draw;
    private PoiSearch poiSearch;
    private NaviRoute naviRoute;

    //****************************************************************************//
    //  ftmap SDK 可以分五个部分：地图、定位、导航
    //****************************************************************************//

    //****************************************************************************//
    //  ftmap SDK 地图
    //****************************************************************************//

    /**
     * 地图UI设置
     */
//    public UiSettings getUiSettings() {
//        if (this.uiSettings == null) {
//            this.uiSettings = new UiSettings();
//        }
//
//        return this.uiSettings;
//    }

    /**
     * 地图状态设置
     */
//    public StateSettings getStateSettings() {
//        if (this.stateSettings == null) {
//            this.stateSettings = new StateSettings();
//        }
//
//        return this.stateSettings;
//    }

    /**
     * 地图交互
     */
//    public ClickListener getClickListener() {
//        if (this.clickListener == null) {
//            this.clickListener = new ClickListener();
//        }
//        return this.clickListener;
//    }

    /**
     * 地图绘制
     */
//    public Draw getDraw() {
//        if (this.draw == null) {
//            this.draw = new Draw();
//        }
//        return this.draw;
//    }

    /**
     * POI搜索
     */
//    public PoiSearch getPoiSearch() {
//        if (this.poiSearch == null) {
//            this.poiSearch = new PoiSearch();
//        }
//        return this.poiSearch;
//    }

    /**
     * 路径规划
     */
    public NaviRoute getNaviRoute() {
        if (this.naviRoute == null) {
            this.naviRoute = new NaviRoute();
        }
        return this.naviRoute;
    }




    public class NaviRoute {

    }

    public interface PoiSearch {
        //关键字搜索
        public void KeywordSearch();

        //类型搜索
        public void kindSearch();

        //范围搜素
        public void AroundSearch();

    }

    public abstract void setClickListener(ClickListener clickListener);

    public interface ClickListener {
        //地图状态改变监听
        public void OnMapStatusChangeListener();

        //注册地图点击事件，用于捕获地图上的点击操作
        void OnMapClickListener(double x, double y);

        //注册地图双击事件，用于捕获地图上的双击操作
        public void OnMapDoubleClickListener();

        //注册地图长按事件，用于用于捕获地图上的长按操作
        public void OnMapLongClickListener();

        //注册地图滑动事件，用于用于捕获地图上的滑动操作
        public void OnScrollGesturesListener();
    }

    public interface Draw {
        //绘制点标记
        public void addMarker();

        //绘制线
        public void addPolyline();

        //绘制面
        public void addCircle();
    }

    public interface StateSettings {
//        private Object latLng;
//        private double zoom;
//        private double minZoom;
//        private double maxZoom;
//        private int tilt;
//        private int bearing;
//        private Object style;
        //设置/获取地图中心点

        //设置地图中心点：
//        public String id();
//        public void id(String id);
        //        LatLng latLng = new LatLng(39.897524, 116.356608);// 地图中心经纬度坐标
        public void setCenterPoint(Object latLng);

        //获取地图中心点：

        public Object getCenterPoint();

        //设置/获取地图缩放级别

        //设置地图缩放级别：

        public void setZoom(double zoom);

        //获取地图缩放级别：
        public double getZoom();

        //设置/获取地图最小缩放级别

        //设置地图最小缩放级别

        public void setMinZoomPreference(double minZoom);

        //获取地图最小缩放级别

        public double getMinZoomLevel();

        //设置/获取地图最大缩放级别

        //设置地图最大缩放级别

        public void setMaxZoomPreference(double maxZoom);

        //获取地图最大缩放级别

        public double getMaxZoomLevel();

        //设置/获取地图俯仰角度

        //设置地图俯仰角度：

        public void setTilt(int tilt);

        //获取地图俯仰角度：

        public int getTilt();

        //设置/获取地图旋转角度

        //设置地图旋转角度：

        public void setBearing(int bearing);

        //获取地图旋转角度：

        public int getBaering();

        //设置/获取地图样式

        //设置地图样式

        public void setStyle(Object style);

        //获取地图样式

        public Object getStyle();


        //从指定的url异步加载新的地图样式。
    }


    public interface UiSettings {
//        private Drawable compass;
//        private boolean rotateGesturesEnabled;
//        private boolean tiltGesturesEnabled;
//        private boolean zoomGesturesEnabled;
//        private boolean zoomControlsEnabled;
//        private boolean doubleTapGesturesEnabled;
//        private boolean scrollGesturesEnabled;

        //设置指南针
        //设置显示和隐藏指南针
        public void setCompassEnabled(boolean compassEnabled);

        //设置指南针图像
        public void setCompassImage(Drawable compass);

        //获取指南针图像
        public Drawable getCompassImage();

        //设置指南针边距
        public void setCompassMargins(int left, int top, int right, int bottom);

        //设置指南针停泊位置
        public void setCompassGravity(int gravity);
        //设置显示和隐藏LOGO

        public void setLogoEnabled(boolean enabled);

        //设置/获取旋转地图

        //设置地图是否可以旋转

        public void setRotateGesturesEnabled(boolean rotateGesturesEnabled);

        //获取地图是否可以旋转

        public boolean isRotateGesturesEnabled();

        //设置/获取倾斜地图

        //设置地图是否可以倾斜

        public void setTiltGesturesEnabled(boolean tiltGesturesEnabled);

        //获取地图是否可以倾斜

        public boolean isTiltGesturesEnabled();

        //设置/获取缩放地图

        //设置地图是否可以缩放

        public void setZoomGesturesEnabled(boolean zoomGesturesEnabled);

        //获取地图是否可以缩放

        public boolean isZoomGesturesEnabled();

        //设置是否启用缩放控件

        public void setZoomControlsEnabled(boolean zoomControlsEnabled);

        //获取是否启用缩放控件

        public boolean isZoomControlsEnabled();
        //设置/获取双指放大地图

        //设置地图是否可以双指放大

        public void setDoubleTapGesturesEnabled(boolean doubleTapGesturesEnabled);


        //获取地图是否可以双指放大

        public boolean isDoubleTapGesturesEnabled();

        //设置/获取滚动地图

        //设置地图是否可以滚动

        public void setScrollGesturesEnabled(boolean scrollGesturesEnabled);

        //获取地图是否可以滚动

        public boolean isScrollGesturesEnabled();

        //设置所有手势

        //设置启用或禁用所有手势

//        public void setAllGesturesEnabled(boolean enabled) {
//            this.setScrollGesturesEnabled(enabled);
//            this.setRotateGesturesEnabled(enabled);
//            this.setTiltGesturesEnabled(enabled);
//            this.setZoomGesturesEnabled(enabled);
//            this.setDoubleTapGesturesEnabled(enabled);
//        }


        //获取地图视图大小

        //获取地图视图的宽度

        public float getWidth();

        //获取地图视图的高度

        public float getHeight();
    }


    /**
     * 搜索结果回调lambada函数
     */
    public interface SearchResultsCallback {
        public void op(JSONArray results);
    }

    /**
     * 参数内容如下:
     * JSONObject{
     * type:
     * result:
     * }
     */
    public interface RoutingResultsCallback {
        public void op(JSONObject result);
    }

    public interface MapUtilsResultsCallback {
        public void op(JSONObject result);
    }

    public interface ClickListenerCallback {
        public void op(JSONObject result);
    }

    /**
     * poi 分类回调lambada函数
     */
    public interface CatagoriesCallback {
        public void op(JSONObject objData);
    }

    /**
     * 点类型
     */
    public static class Point extends Object {
        public double x;
        public double y;

        public Point(double x, double y) {
            this.x = x;
            this.y = y;
        }
    }

    /**
     * 绘制对象
     */
    public interface DrawItem {
        public String id();

        public void id(String id);

        public String color();

        public void color(String v);
        public String icon();

        public void icon(String v);

        public boolean visible();

        public void visible(boolean v);

        public void destroy();

        public void update();
    }

    /**
     * 点绘制对象
     */
    public interface PointItem extends DrawItem {
        public void pos(Point p);

        public Point pos();

        public void radius(double v);

        public double radius();

        public void shape(String v);

        public String shape();
    }

    /**
     * 线绘制对象
     */
    public interface LineItem extends DrawItem {
        public void points(JSONArray points);

        public JSONArray points();

        public void width(double v);

        public double width();
    }

    /**
     * 面绘制对象
     */
    public interface AreaItem extends DrawItem {
        public void points(List<Point> points);

        public List<Point> points();
    }

    public enum DrawItemType {
        POINT, LINE, AREA
    }

    //****************************************************************************//
    //  接口方法
    //****************************************************************************//

    /**
     * 创建绘制对象
     *
     * @param type
     * @return
     */
    public abstract DrawItem createDrawItem(DrawItemType type);


    public abstract ClickListener createClickListener();

    /**
     * 初始化引擎
     *
     * @param app
     * @param ctx
     * @param mapResId
     * @param appId
     * @return 成功为true，否则false
     */
    public abstract boolean init(Application app, FragmentActivity ctx, int mapResId, String appId);

    /**
     * 普通查询搜索
     *
     * @param keywords
     * @param callback
     */
    public abstract void search(String keywords, SearchResultsCallback callback);

    /**
     * 圆形范围检索
     *
     * @param keywords
     * @param centerX
     * @param centerY
     * @param radius
     * @param callback
     */
    public abstract void search(String keywords, double centerX, double centerY, double radius, SearchResultsCallback callback);


    public abstract void PtoG(double screenX, double screenY, MapUtilsResultsCallback callback);

    public abstract void ScreenToMapObject(double screenX, double screenY, MapUtilsResultsCallback callback);

    public abstract void ToLatLon(double mercatorX, double mercatorY, MapUtilsResultsCallback callback);

    public abstract void FromLatLon(double lat, double lon, MapUtilsResultsCallback callback);

    public abstract void GetFeatureID(double mercatorX, double mercatorY, MapUtilsResultsCallback callback);

    public abstract void GetMapObject(double mercatorX, double mercatorY, MapUtilsResultsCallback callback);

    public abstract void OnMapClickListener(ClickListenerCallback callback);

    /**
     * 查询poi分类
     * @param callback
     */
//    public abstract void queryPoiCatagories(CatagoriesCallback callback);

    /**
     * 地图缩放
     *
     * @param isIn isIn为true代表放大，否则为缩小
     */
    public abstract void zoom(boolean isIn);

    /**
     * 定位到指定坐标
     *
     * @param lat  为0 代表当前定位位置
     * @param lon  为0 代表当前定位位置
     * @param zoom 缩放级别，-1代表保持当前级别不变
     */
    public abstract void setViewCenter(double lat, double lon, int zoom);

    public abstract void showRoute(String routeId, String fillColor, String outlineColor);
    public abstract void hideRoute(String routeId);

    public abstract void nativeSetupWidget(int widget, float x, float y, int anchor, SearchResultsCallback callback);

    /**
     * 将点集合全部显示到视图中间
     *
     * @param points
     */
    public abstract void centerPoints(JSONArray points);

    public abstract void centerPoints(List<Point> points);


    /**
     *删除导航
     */

    public abstract void removeRoute();
    /**
     *开始导航，视角跟随
     */

    public abstract void followRoute();
    /**
     *结束导航
     */

    public abstract void CloseRouting();

    /**
     * 路径规划
     *
     * @param points   [{x:0,y:0},{x:0,y:0},{x:0,y:0}]
     * @param type     交通方式:car, bycle, walk
     * @param callback
     */
    public abstract void routing(JSONArray points, String type, RoutingResultsCallback callback);

    /**
     * 改变地图风格
     *
     * @param style
     */
    public abstract void setMapStyle(String style);



//    public abstract
}
