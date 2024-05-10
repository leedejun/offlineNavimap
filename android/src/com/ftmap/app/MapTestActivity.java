//
// Source code recreated from a .class file by IntelliJ IDEA
// (powered by FernFlower decompiler)
//

package com.ftmap.app;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.location.Location;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemSelectedListener;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.ftmap.base.MediaPlayerWrapper;
import com.ftmap.maps.BuildConfig;
import com.ftmap.maps.FMap;
import com.ftmap.maps.FTMap;
import com.ftmap.maps.FTMapRoutePoint;
import com.ftmap.maps.HotelsFilter;
import com.ftmap.maps.MapRenderingListener;
import com.ftmap.maps.R;
import com.ftmap.maps.RoutingInfo;
import com.ftmap.maps.FMap.DrawItemType;
import com.ftmap.maps.FMap.Point;
import com.ftmap.maps.FMap.PointItem;
import com.ftmap.maps.FTMapRoutePoint.FTMapPoint;
import com.ftmap.maps.R.id;
import com.ftmap.maps.R.layout;
import com.ftmap.maps.location.CompassData;
import com.ftmap.maps.location.LocationHelper;
import com.ftmap.maps.location.LocationListener;
import com.ftmap.maps.location.LocationHelper.UiCallback;
import com.ftmap.maps.location.LocationState.ModeChangeListener;
import com.ftmap.maps.search.BookingFilterParams;
import com.ftmap.maps.search.NativeSearchListener;
import com.ftmap.maps.search.SearchEngine;
import com.ftmap.maps.search.SearchResult;
import com.ftmap.maps.sound.TtsPlayer;

import java.util.ArrayList;
import java.util.Map;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.ftmap.maps.location.LocationState;

public class MapTestActivity extends AppCompatActivity implements OnClickListener, MapRenderingListener, LocationListener, ModeChangeListener, NativeSearchListener, UiCallback {
    private static final String TAG = MapTestActivity.class.getSimpleName();
    private PointItem point = null;
    private EditText etSearchPoi;
    private Spinner mapZoomSp;
    private TextView mapZoomTv;
    private Boolean firstEnter = false;
    private int mMyPositionMode;
    private  MediaPlayerWrapper mPlayer;
    private MapTestActivity.MapButtonClickListener mMapButtonClickListener;

    public MapTestActivity() {
    }

    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ftmap);

        FMap.INSTANCE.init(this.getApplication(), this, R.id.map_view_container, BuildConfig.APPLICATION_ID);

        LocationHelper.INSTANCE.onEnteredIntoFirstRun();
        if (!LocationHelper.INSTANCE.isActive())
            LocationHelper.INSTANCE.start();
        LocationHelper.INSTANCE.addListener(this);
        mPlayer = new MediaPlayerWrapper(this.getApplication());
        FMap.INSTANCE.FromLatLon(0.0,0.0,(results) -> {

        });

//        this.initOnMapClickListener();
        this.findViewById(id.btnSearchPoi).setOnClickListener(this);
        this.findViewById(id.btnLocate).setOnClickListener(this);
        this.findViewById(id.btnRoute).setOnClickListener(this);
        this.findViewById(id.btnZoomIn).setOnClickListener(this);
        this.findViewById(id.btnZoomOut).setOnClickListener(this);
        this.findViewById(id.btnStyleClear).setOnClickListener(this);
        this.findViewById(id.btnStyleDark).setOnClickListener(this);
        this.findViewById(id.btShowRoute).setOnClickListener(this);
        this.findViewById(id.btHideRoute).setOnClickListener(this);
        this.findViewById(id.btFollowRoute).setOnClickListener(this);
        this.etSearchPoi = (EditText) this.findViewById(id.etSearchPoi);
        this.mapZoomSp = (Spinner) this.findViewById(id.map_zoom_sp);
        this.mapZoomTv = (TextView) this.findViewById(id.map_zoom_tv);
        ArrayList<String> zoomStr = new ArrayList();

        for (int i = 0; i < 19; ++i) {
            int zoom = 1 + i;
            zoomStr.add(zoom + "");
        }
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, zoomStr);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);


        this.mapZoomSp.setAdapter(adapter);
        this.mapZoomSp.setVisibility(View.GONE);
        this.mapZoomSp.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (MapTestActivity.this.firstEnter) {
                    DisplayMetrics dm = new DisplayMetrics();
                    MapTestActivity.this.getWindowManager().getDefaultDisplay().getRealMetrics(dm);
                    int dwidth = dm.widthPixels;
                    int dheight = dm.heightPixels;
                    FMap.INSTANCE.PtoG((double) (dwidth / 2), (double) (dheight / 2), (results1) -> {
                        try {
                            double mercatorX = results1.getDouble("mercatorX");
                            double mercatorY = results1.getDouble("mercatorY");
                            FMap.INSTANCE.ToLatLon(mercatorX, mercatorY, (result1) -> {
                                try {
                                    double lat = result1.getDouble("lat");
                                    double lon = result1.getDouble("lon");
                                    FMap.INSTANCE.setViewCenter(lat, lon, position + 1);
                                } catch (JSONException var6) {
                                    var6.printStackTrace();
                                }

                            });
                        } catch (JSONException var6) {
                            var6.printStackTrace();
                        }

                    });
                }

                MapTestActivity.this.firstEnter = true;
            }

            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
        String a = "提交11";
    }
    @Override
    protected void onStart() {
        super.onStart();
    }
    @Override
    protected void onStop() {
        super.onStop();
    }
    @Override
    protected void onResume() {
        FTMap.nativeOnTransit(true);
        super.onResume();
    }
    @Override
    protected void onPause() {
        FTMap.nativeOnTransit(false);
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        FTMap.nativeOnTransit(false);
        super.onDestroy();
    }

    private void runSearch(String keyword) {
        SearchEngine.INSTANCE.cancel();
        SearchEngine.INSTANCE.searchInteractive(keyword, System.nanoTime(), true, (HotelsFilter) null, (BookingFilterParams) null);
        SearchEngine.INSTANCE.setQuery(keyword);
    }

    public void onResultsUpdate(@NonNull SearchResult[] results, long timestamp, boolean isHotel) {
        Log.d("Search111", results.toString());
    }

    public void onResultsEnd(long timestamp) {
        Log.d("Search", "onResultsEnd");
    }

    @SuppressLint({"NonConstantResourceId"})
    public void onClick(View view) {
        int id = view.getId();
        if (id == R.id.btFollowRoute) {
//            FMap.INSTANCE.removeRoute();
//            FMap.INSTANCE.closeRouting();
//            FTMap.nativeDisableFollowing();
//            FTMap.nativeDeleteSavedRoutePoints();
//            FTMap.nativeCloseRouting();
//            FMap.INSTANCE.updatePreviewModeAll();
            //该函数为native函数
//            03300356c92.png
//           FMap.INSTANCE.addCustomMark("1","/storage/emulated/0/MapsWithMe/pictures/xushuichi.png",
//                   39.988965,116.3676281,"fdtest_poi", "#00FF00",16);

            Log.d("FMap.AreaItem", "1234567");

            FMap.AreaItem areaItem = (FMap.AreaItem) FMap.INSTANCE.createDrawItem(FMap.DrawItemType.AREA);
            String aa = "116.35772,39.99226;116.358599,39.992297;116.358888,39.992307;116.359097,39.992317;116.359706,39.992325;116.360235,39.992344;116.361254,39.992373;116.36153,39.992377;116.361531,39.992377";
            StringBuilder stringBuffer = new StringBuilder();
            stringBuffer.append("[");
            stringBuffer.append("116.35772");
            stringBuffer.append(",");
            stringBuffer.append("39.99226");
            stringBuffer.append(",");
            stringBuffer.append("116.361531");
            stringBuffer.append(",");
            stringBuffer.append("39.992377");
            stringBuffer.append(",");
            stringBuffer.append("116.398888");
            stringBuffer.append(",");
            stringBuffer.append("39.912307");
            stringBuffer.append("]");
            JSONArray  array = null;
            try {
                array = new JSONArray(stringBuffer.toString());
            } catch (JSONException e) {
                e.printStackTrace();
            }
            areaItem.points(array);
            areaItem.outlineWidth(6.0);
            areaItem.outlineColor("#000000");
            areaItem.color("#55ffff55");
            areaItem.id("11222");
            long uplinedate = areaItem.upareadate();
//            int zoomlevel = FMap.INSTANCE.getZoomlevel();
//            Log.d("zoomlevel", zoomlevel+"");
//            FMap.INSTANCE.LatLonToMapObject(39.988965415846174,116.36762810740483,(JSONObject results2) -> {
//                try {
//                    String poiName = results2.getString("poiName");
//                    String poiAddress = results2.getString("poiAddress");
//
//                } catch (JSONException e) {
//                    e.printStackTrace();
//                }
//            });
//            FMap.INSTANCE.followRoute("route-1");
        } else if (id == R.id.btnSearchPoi) {
            String aa = "116.35772,39.99226;116.358599,39.992297;116.358888,39.992307;116.359097,39.992317;116.359706,39.992325;116.360235,39.992344;116.361254,39.992373;116.36153,39.992377;116.361531,39.992377";
            String bb = "116.344908,39.985187;116.344908,39.985187;116.344916,39.985074;116.345086,39.985081;116.346582,39.985129;116.346574,39.985238;116.346564,39.985408;116.346564,39.985558;116.346553,39.985698;116.344458,39.985651;116.344268,39.985642;116.343503,39.985624;116.342653,39.985605;116.342352,39.985605;116.342014,39.985595;116.341744,39.985587;116.341663,39.985587;116.340647,39.985558;116.339737,39.985529;116.339311,39.985521;116.338710,39.985503;116.338442,39.985562;116.338251,39.985583;116.338112,39.985573;116.337195,39.985554;116.335768,39.985499;116.334853,39.985460;116.334612,39.985449;116.334561,39.985460;116.334451,39.985460;116.333442,39.985472;116.333075,39.985464;116.332595,39.985464;116.332514,39.985484;116.332455,39.985534;116.332404,39.985684;116.332404,39.985725;116.332356,39.986195;116.332326,39.986345;116.332320,39.986382;116.332205,39.986370;";
            String s = this.etSearchPoi.getText().toString();
            FMap.INSTANCE.cancelSearch();
            FMap.INSTANCE.search(s, 39.985187, 116.344908, (results) -> {
                String s1 = results.toString();
                Log.d("Search22", s1);
            });
//            int zoomlevel = FMap.INSTANCE.getZoomlevel();
//            Log.d("zoomlevel", zoomlevel+"");
        } else if (id == R.id.btShowRoute) {
            ArrayList<Long> objects = new ArrayList<>();
            objects.add(1l);
            objects.add(2l);
//            FMap.INSTANCE.removeCustomMark(objects);
//            FMap.INSTANCE.showRoute("route-0", "#0000ff", "#00ff00");
            FMap.INSTANCE.showRoute("route-1", "#ff0000", "#00ff00");
//            FMap.INSTANCE.showRoute("route-2", "#00ff00", "#00ff00");

        } else if (id == R.id.btHideRoute) {
            FMap.INSTANCE.removeAreaItem(11222l);
//            FMap.INSTANCE.updatePreviewModeAll();
//            RoutingInfo mRoutingInfo=      FTMap.nativeGetRouteFollowingInfo();
//                FMap.INSTANCE.getRouteTime("route-1", (result) -> {
//                    String a = result.toString();
//                    Log.d("getRouteTime", a);
//                });
//                FMap.INSTANCE.getRouteDistance("route-1", (result) -> {
//                    String a1 = result.toString();
//                    Log.d("getRouteDistance", a1);
//                });
//                FMap.INSTANCE.getRouteInfo("route-1", (result) -> {
//                    String a1 = result.toString();
//                    Log.d("getRouteInfo", a1);
//                });
        } else if (id == R.id.btnLocate) {
//            PointItem pointItem = (PointItem) FMap.INSTANCE.createDrawItem(DrawItemType.POINT);
//            pointItem.pos(new Point(116.40152D, 39.90768D));
//            pointItem.color("#00FF00");
//            pointItem.id("111111");
//            pointItem.icon("ftmap-chechang");
//            pointItem.radius(100.0D);
//            TtsPlayer.setEnabled(false);

          String name  =  FTMap.nativeFindCountry(39.90768D, 116.40152D);
          String name1  =  FTMap.nativeFindCountry(43.8339670D, 87.5712401D);
            Log.d("nativeFindCountry", name+"---"+name1);
//            long poiIdBack = pointItem.uppoidate();
//            LocationState.nativeSwitchToNextMode();
//            if (!LocationHelper.INSTANCE.isActive())
//                LocationHelper.INSTANCE.start();
//            FMap.INSTANCE.setMyDefaultPosition(39.90768D, 116.40152D);
        } else if (id == R.id.btnRoute) {

//            43.8339670,87.5712401  39.552547,75.953821
//
//            37.36986,79.89765   41.7908,94.0441
//
//            46.86766,87.67186	37.12927,79.50925
//
//            45.18325,89.36705		37.889954,77.554389
//
//            42.97477,90.36613		39.51416,76.30483
//
//            43.940892,87.612627		41.21360,80.27815

            FMap.INSTANCE.removeRoute();
            FMap.INSTANCE.closeRouting();
            FTMapRoutePoint mFTMapRoutePoint = new FTMapRoutePoint();
            FTMapPoint startPoint = new FTMapPoint();
            Location lastLocation = LocationHelper.INSTANCE.getLastKnownLocation();
            startPoint.setName("我的位置");
          // 103.497729,36.141007
            startPoint.setLat(36.141007D);
            startPoint.setLon(103.497729D);
//            startPoint.setLat(43.8339670);
//            startPoint.setLon(87.5712401 );
//            startPoint.setLat(lastLocation.getLatitude());
//            startPoint.setLon(lastLocation.getLongitude());
            startPoint.setMyPosition(true);
            FTMapPoint endPoint = new FTMapPoint();
            endPoint.setName("北京饭店");
            //103.939793,36.044013
            endPoint.setLat(36.044013D);
            endPoint.setLon(103.939793D);
//            endPoint.setLat(39.90768D);
//            endPoint.setLon(116.40152D);
            endPoint.setMyPosition(false);
            mFTMapRoutePoint.setEndPoint(endPoint);
            mFTMapRoutePoint.setStartPoint(startPoint);
            JSONArray points = FTMapRoutePoint.toJsonArr();
            FMap.INSTANCE.routing(points, "Vehicle", (result) -> {
                String s1 = result.toString();
                Log.d("routing", s1);
//                FMap.INSTANCE.updatePreviewModeAll();
                try {
                    JSONArray var2 = result.getJSONArray("result");
                } catch (Exception var3) {
                }

            });
        } else if (id == R.id.btnZoomIn) {
//                            FMap.INSTANCE.getRouteInfo("route-1", (result) -> {
//                    String a1 = result.toString();
//                    Log.d("getRouteInfo", a1);
//                });
//            RoutingInfo routingInfo = FTMap.nativeGetRouteFollowingInfo();
//            Log.d("routingInfo", routingInfo.nextStreet);
//                bb = routingInfo.toString();
            TtsPlayer.INSTANCE.playTurnNotifications(mPlayer);
        } else if (id == R.id.btnZoomOut) {
            FMap.INSTANCE.zoom(false);
            TtsPlayer.setEnabled(true);
        } else if (id == R.id.btnStyleClear) {
            FMap.INSTANCE.setMapStyle("clear");
        } else if (id == R.id.btnStyleDark) {
            FMap.INSTANCE.setMapStyle("camouflage");
        }


    }

    private void initOnMapClickListener() {
        FMap.INSTANCE.OnMapClickListener((results) -> {
            String s1 = results.toString();
            Log.d("OnMapClickListener", s1);

            try {
                if ("longClick".equals(results.get("clickTypr").toString())) {
                    int screenX = results.getInt("screenX");
                    int screenY = results.getInt("screenY");
                    FMap.INSTANCE.PtoG((double) screenX, (double) screenY, (results1) -> {
                        try {
                            double mercatorX = results1.getDouble("mercatorX");
                            double mercatorY = results1.getDouble("mercatorY");
                            FMap.INSTANCE.GetMapObject(mercatorX, mercatorY, (results2) -> {
                                Toast.makeText(this, results2.toString(), Toast.LENGTH_LONG).show();
                            });
                        } catch (JSONException var6) {
                            var6.printStackTrace();
                        }

                    });
                } else if ("Move".equals(results.get("clickTypr").toString())) {
                    DisplayMetrics dm1 = this.getResources().getDisplayMetrics();
                    FMap.INSTANCE.ScreenToMapObject((double) (dm1.widthPixels / 2), (double) (dm1.heightPixels / 2), (results1) -> {
                    });
                }
            } catch (JSONException var5) {
                var5.printStackTrace();
            }

        });
    }

    public void onRenderingCreated() {
//        LocationHelper.INSTANCE.attach(this);
    }

    public void onRenderingRestored() {
    }

    public void onRenderingInitializationFinished() {
    }

    public void onPointerCaptureChanged(boolean hasCapture) {
        super.onPointerCaptureChanged(hasCapture);
    }

    public Activity getActivity() {
        return null;
    }

    public void onMyPositionModeChanged(int newMode) {
    }

    public void onLocationUpdated(@NonNull Location location) {
        RoutingInfo routingInfo = FTMap.nativeGetRouteFollowingInfo();
        if (routingInfo != null) {
            String s = routingInfo.toString();

            String distToTurn = routingInfo.distToTurn;
            String turnUnits = routingInfo.turnUnits;
            String nextStreet = routingInfo.nextStreet;
//            String distToTarget = routingInfo.distToTarget;
//            int totalTimeInSeconds = routingInfo.totalTimeInSeconds;
//            String targetUnits = routingInfo.targetUnits;
            Log.d("移动了定位", s);
            Toast.makeText(this, distToTurn+"----"+turnUnits+"---"+nextStreet, Toast.LENGTH_LONG).show();
        }
        if (FTMap.nativeIsRouteFinished()) {

            Log.d("导航结束", "");
        }else{
            Log.d("导航中", "");
        }
        TtsPlayer.INSTANCE.playTurnNotifications(mPlayer);
    }

    public void onCompassUpdated(long time, double north) {
        FMap.INSTANCE.nativeCompassUpdated(north,false);

    }

    public void onLocationError(int errorCode) {
    }

    public void onCompassUpdated(@NonNull CompassData compass) {
    }

    public void onLocationError() {
    }

    public void onLocationNotFound() {
    }

    public void onRoutingFinish() {
    }

    public interface MapButtonClickListener {
        void onClick(MapTestActivity.MapButtons var1);
    }

    public static enum MapButtons {
        myPosition,
        toggleMapLayer,
        zoomIn,
        zoomOut,
        zoom,
        search,
        bookmarks,
        menu,
        help;

        private MapButtons() {
        }
    }

}
