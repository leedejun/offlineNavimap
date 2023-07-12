package com.ftmap.maps;

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Rect;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;

import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.ftmap.maps.location.LocationHelper;
import com.ftmap.util.concurrency.UiThread;
import com.ftmap.util.log.Logger;
import com.ftmap.util.log.LoggerFactory;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class FTMapFragment extends Fragment
        implements View.OnTouchListener,
        SurfaceHolder.Callback {
    public static final String ARG_LAUNCH_BY_DEEP_LINK = "launch_by_deep_link";
    private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    private static final String TAG = FTMapFragment.class.getSimpleName();

    // Should correspond to android::MultiTouchAction from Framework.cpp
    private static final int NATIVE_ACTION_UP = 0x01;
    private static final int NATIVE_ACTION_DOWN = 0x02;
    private static final int NATIVE_ACTION_MOVE = 0x03;
    private static final int NATIVE_ACTION_CANCEL = 0x04;

    // Should correspond to gui::EWidget from skin.hpp
    private static final int WIDGET_RULER = 0x01;
    private static final int WIDGET_COMPASS = 0x02;
    private static final int WIDGET_COPYRIGHT = 0x04;
    private static final int WIDGET_SCALE_FPS_LABEL = 0x08;
    private static final int WIDGET_WATERMARK = 0x10;

    // Should correspond to dp::Anchor from drape_global.hpp
    private static final int ANCHOR_CENTER = 0x00;
    private static final int ANCHOR_LEFT = 0x01;
    private static final int ANCHOR_RIGHT = (ANCHOR_LEFT << 1);
    private static final int ANCHOR_TOP = (ANCHOR_RIGHT << 1);
    private static final int ANCHOR_BOTTOM = (ANCHOR_TOP << 1);
    private static final int ANCHOR_LEFT_TOP = (ANCHOR_LEFT | ANCHOR_TOP);
    private static final int ANCHOR_RIGHT_TOP = (ANCHOR_RIGHT | ANCHOR_TOP);
    private static final int ANCHOR_LEFT_BOTTOM = (ANCHOR_LEFT | ANCHOR_BOTTOM);
    private static final int ANCHOR_RIGHT_BOTTOM = (ANCHOR_RIGHT | ANCHOR_BOTTOM);

    // Should correspond to df::TouchEvent::INVALID_MASKED_POINTER from user_event_stream.cpp
    private static final int INVALID_POINTER_MASK = 0xFF;
    private static final int INVALID_TOUCH_ID = -1;
    private static FMap.ClickListenerCallback callback;

    //    private static long firstClickTime;
//    private static long secondClickTime;
//    private static long stillTime;
//    private static float firstClickX;
//    private static float firstClickY;
//    private static boolean isUp = false;
//    private static boolean isDoubleClick = false;
    private int mClickcount;// 点击次数
    private int mDownX;
    private int mDownY;
    private int mMoveX;
    private int mMoveY;
    private int mUpX;
    private int mUpY;
    private long mLastDownTime;
    private long mLastUpTime;
    private long mFirstClick;
    private long mSecondClick;
    private boolean isDoubleClick = false;
    private int MAX_LONG_PRESS_TIME = 350;// 长按/双击最长等待时间
    private int MAX_SINGLE_CLICK_TIME = 50;// 单击最长等待时间
    private int MAX_MOVE_FOR_CLICK = 50;// 最长改变距离,超过则算移动

    private Handler mBaseHandler = new Handler();

    private Runnable mLongPressTask = new Runnable() {
        @Override
        public void run() {
            //处理长按

            JSONObject jsonObject = new JSONObject();
            try {
                jsonObject.put("clickTypr", "longClick");
                jsonObject.put("screenX", mDownX);
                jsonObject.put("screenY", mDownY);
                MapClickListener.Callback(jsonObject);
            } catch (JSONException e) {
                e.printStackTrace();
            }
            mClickcount = 0;
        }
    };

    private Runnable mSingleClickTask = new Runnable() {
        @Override
        public void run() {
            // 处理单击
            JSONObject jsonObject = new JSONObject();
            try {
                jsonObject.put("clickTypr", "Click");
                jsonObject.put("screenX", mUpX);
                jsonObject.put("screenY", mUpY);
                MapClickListener.Callback(jsonObject);
            } catch (JSONException e) {
                e.printStackTrace();
            }
            mClickcount = 0;
        }
    };
    private Runnable mMoveClickTask = new Runnable() {
        @Override
        public void run() {
            // 处理单击
            JSONObject jsonObject = new JSONObject();
            try {
                jsonObject.put("clickTypr", "Move");
                jsonObject.put("screenX", mUpX);
                jsonObject.put("screenY", mUpY);
                MapClickListener.Callback(jsonObject);
            } catch (JSONException e) {
                e.printStackTrace();
            }
            mClickcount = 0;
        }
    };

    private int mHeight;
    private int mWidth;
    private boolean mRequireResize;
    private boolean mSurfaceCreated;
    private boolean mSurfaceAttached;
    private boolean mLaunchByDeepLink;
    private static boolean sWasCopyrightDisplayed;
    @Nullable
    private String mUiThemeOnPause;
    @SuppressWarnings("NullableProblems")
    @NonNull
    private SurfaceView mSurfaceView;
//  private FMap.ClickListenerCallback callback;
//  @Nullable
  private MapRenderingListener mMapRenderingListener;

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
//    if (isThemeChangingProcess())
//    {
//      LOGGER.d(TAG, "Activity is being recreated due theme changing, skip 'surfaceCreated' callback");
//      return;
//    }

        LOGGER.d(TAG, "surfaceCreated, mSurfaceCreated = " + mSurfaceCreated);
        final Surface surface = surfaceHolder.getSurface();
        if (nativeIsEngineCreated()) {
            if (!nativeAttachSurface(surface)) {
                return;
            }
            mSurfaceCreated = true;
            mSurfaceAttached = true;
            mRequireResize = true;
            nativeResumeSurfaceRendering();
            return;
        }

        mRequireResize = false;

        final DisplayMetrics metrics = new DisplayMetrics();
//        requireActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
        //适配低版本
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
        final float exactDensityDpi = metrics.densityDpi;

        final boolean firstStart = true;//MwmApplication.from(requireActivity()).isFirstLaunch();
        if (!nativeCreateEngine(surface, (int) exactDensityDpi, firstStart, mLaunchByDeepLink,
                0)) {
            return;
        }

        if (firstStart) {
            UiThread.runLater(new Runnable() {
                @Override
                public void run() {
                    LocationHelper.INSTANCE.onExitFromFirstRun();
                }
            });
        }

        mSurfaceCreated = true;
        mSurfaceAttached = true;
        nativeResumeSurfaceRendering();
    if (mMapRenderingListener != null)
      mMapRenderingListener.onRenderingCreated();
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height) {
//    if (isThemeChangingProcess())
//    {
//      LOGGER.d(TAG, "Activity is being recreated due theme changing, skip 'surfaceChanged' callback");
//      return;
//    }

        LOGGER.d(TAG, "surfaceChanged, mSurfaceCreated = " + mSurfaceCreated);
        if (!mSurfaceCreated || (!mRequireResize && surfaceHolder.isCreating()))
            return;

        final Surface surface = surfaceHolder.getSurface();
        nativeSurfaceChanged(surface, width, height);

        mRequireResize = false;
    if (mMapRenderingListener != null)
      mMapRenderingListener.onRenderingRestored();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        LOGGER.d(TAG, "surfaceDestroyed");
        destroySurface();
    }

    void destroySurface() {
        LOGGER.d(TAG, "destroySurface, mSurfaceCreated = " + mSurfaceCreated +
                ", mSurfaceAttached = " + mSurfaceAttached + ", isAdded = " + isAdded());
        if (!mSurfaceCreated || !mSurfaceAttached || !isAdded())
            return;

//        nativeDetachSurface(!requireActivity().isChangingConfigurations());
        //适配低版本
        nativeDetachSurface(!getActivity().isChangingConfigurations());
        mSurfaceCreated = !nativeDestroySurfaceOnDetach();
        mSurfaceAttached = false;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
    mMapRenderingListener = (MapRenderingListener) context;
    }

    @Override
    public void onDetach() {
        super.onDetach();
    mMapRenderingListener = null;
    }

    @Override
    public void onCreate(Bundle b) {
        super.onCreate(b);
        setRetainInstance(true);
        Bundle args = getArguments();
        if (args != null)
            mLaunchByDeepLink = args.getBoolean(ARG_LAUNCH_BY_DEEP_LINK);
    }

    @Override
    public void onStart() {
        super.onStart();
//    nativeSetRenderingInitializationFinishedListener(mMapRenderingListener);
        LOGGER.d(TAG, "onStart");
    }

    public void onStop() {
        super.onStop();
//    nativeSetRenderingInitializationFinishedListener(null);
        LOGGER.d(TAG, "onStop");
    }

//  private boolean isThemeChangingProcess()
//  {
//    return mUiThemeOnPause != null && !mUiThemeOnPause.equals(Config.getCurrentUiTheme());
//  }

    @Override
    public void onPause() {
//    mUiThemeOnPause = Config.getCurrentUiTheme();

        // Pause/Resume can be called without surface creation/destroy.
        if (mSurfaceAttached)
            nativePauseSurfaceRendering();

        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();

        // Pause/Resume can be called without surface creation/destroy.
        if (mSurfaceAttached)
            nativeResumeSurfaceRendering();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_map, container, false);
        mSurfaceView = view.findViewById(R.id.map_surfaceview);
        mSurfaceView.getHolder().addCallback(this);
        return view;
    }

    @Override
    public boolean onTouch(View view, MotionEvent event) {
        final int count = event.getPointerCount();


        if (count == 0)
            return false;

        int action = event.getActionMasked();
        int pointerIndex = event.getActionIndex();
        switch (action) {
            case MotionEvent.ACTION_POINTER_UP:
                action = NATIVE_ACTION_UP;
                break;
            case MotionEvent.ACTION_UP:
                action = NATIVE_ACTION_UP;
                pointerIndex = 0;
                break;
            case MotionEvent.ACTION_POINTER_DOWN:
                action = NATIVE_ACTION_DOWN;
                break;
            case MotionEvent.ACTION_DOWN:
                action = NATIVE_ACTION_DOWN;
                pointerIndex = 0;
                break;
            case MotionEvent.ACTION_MOVE:
                action = NATIVE_ACTION_MOVE;
                pointerIndex = INVALID_POINTER_MASK;
                break;
            case MotionEvent.ACTION_CANCEL:
                action = NATIVE_ACTION_CANCEL;
                break;
        }
        if (action == NATIVE_ACTION_DOWN) {

            mLastDownTime = System.currentTimeMillis();
            mDownX = (int) event.getX();
            mDownY = (int) event.getY();
            mClickcount++;
            Log.e("mouse", "DOWN-->mClickcount=" + mClickcount + "; isDoubleClick=" + isDoubleClick);
            if (mSingleClickTask != null) {
                mBaseHandler.removeCallbacks(mSingleClickTask);
            }
            if (!isDoubleClick) mBaseHandler.postDelayed(mLongPressTask, MAX_LONG_PRESS_TIME);
            if (1 == mClickcount) {
                mFirstClick = System.currentTimeMillis();
            } else if (mClickcount == 2) {// 双击
                mSecondClick = System.currentTimeMillis();
                if (mSecondClick - mFirstClick <= MAX_LONG_PRESS_TIME) {
                    //处理双击
                    JSONObject jsonObject = new JSONObject();
                    try {
                        jsonObject.put("clickTypr", "doubleClick");
                        jsonObject.put("screenX", mDownX);
                        jsonObject.put("screenY", mDownX);
                        MapClickListener.Callback(jsonObject);
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    isDoubleClick = true;
                    mClickcount = 0;
                    mFirstClick = 0;
                    mSecondClick = 0;
                    mBaseHandler.removeCallbacks(mSingleClickTask);
                    mBaseHandler.removeCallbacks(mLongPressTask);
                    Log.e("mouse", "double double double....");
                }
            }


        } else if (action == NATIVE_ACTION_MOVE) {
            mMoveX = (int) event.getX();
            mMoveY = (int) event.getY();
            int absMx = Math.abs(mMoveX - mDownX);
            int absMy = Math.abs(mMoveY - mDownY);
            Log.e("mouse", "MOVE-->absMx=" + absMx + "; absMy=" + absMy);
            if (absMx > MAX_MOVE_FOR_CLICK && absMy > MAX_MOVE_FOR_CLICK) {
                mBaseHandler.removeCallbacks(mLongPressTask);
                mBaseHandler.removeCallbacks(mSingleClickTask);

                isDoubleClick = false;
                mClickcount = 0;//移动了
            }
            if (absMx >= 5 && absMy >= 5) {
                //处理移动

                isDoubleClick = false;
                mClickcount = 0;//移动了
            }

        } else if (action == NATIVE_ACTION_UP) {
            mLastUpTime = System.currentTimeMillis();
            mUpX = (int) event.getX();
            mUpY = (int) event.getY();
            int mx = Math.abs(mUpX - mDownX);
            int my = Math.abs(mUpY - mDownY);
            Log.e("mouse", "UP-->mx=" + mx + "; my=" + my);
            if (mx <= MAX_MOVE_FOR_CLICK && my <= MAX_MOVE_FOR_CLICK) {
                if ((mLastUpTime - mLastDownTime) <= MAX_LONG_PRESS_TIME) {
                    mBaseHandler.removeCallbacks(mLongPressTask);
                    if (!isDoubleClick)
                        mBaseHandler.postDelayed(mSingleClickTask, MAX_SINGLE_CLICK_TIME);
                } else {
                    //超出了双击间隔时间
                    mClickcount = 0;
                }
            } else {
                //移动了
                mBaseHandler.postDelayed(mMoveClickTask, MAX_SINGLE_CLICK_TIME);
                mClickcount = 0;
            }
            if (isDoubleClick) isDoubleClick = false;
        }


        switch (count) {
            case 1:
                nativeOnTouch(action, event.getPointerId(0), event.getX(), event.getY(), INVALID_TOUCH_ID, 0, 0, 0);
                return true;
            default:
                nativeOnTouch(action,
                        event.getPointerId(0), event.getX(0), event.getY(0),
                        event.getPointerId(1), event.getX(1), event.getY(1), pointerIndex);
                return true;
        }
    }

    boolean isContextCreated() {
        return mSurfaceCreated;
    }

    static void nativeCompassUpdated(double north, boolean forceRedraw) {
        FTMap.cmd("compass_update").set("north", north).set("forceRedraw", forceRedraw).run();
    }

    static boolean nativeIsEngineCreated() {
        return (Boolean) FTMap.cmd("IsDrapeEngineCreated").run().get("result");
    }

    static boolean nativeDestroySurfaceOnDetach() {
        return (Boolean) FTMap.cmd("destroySurfaceOnDetach").run().get("result");
    }

    private static boolean nativeCreateEngine(Surface surface, int density,
                                              boolean firstLaunch,
                                              boolean isLaunchByDeepLink,
                                              int appVersionCode) {
        return (Boolean) FTMap.cmd("createEngine")
                .set("surface", surface)
                .set("density", density)
                .set("firstLaunch", firstLaunch)
                .set("isLaunchByDeepLink", isLaunchByDeepLink)
                .set("appVersionCode", appVersionCode)
                .run().get("result");
    }

    private static boolean nativeAttachSurface(Surface surface) {
        return (Boolean) FTMap.cmd("attachSurface")
                .set("surface", surface)
                .run().get("result");
    }

    private static void nativeDetachSurface(boolean destroySurface) {
        FTMap.cmd("detachSurface").set("destroy", destroySurface).run();
    }

    private static void nativePauseSurfaceRendering() {
        FTMap.cmd("pauseSurface").run();
    }

    private static void nativeResumeSurfaceRendering() {
        FTMap.cmd("resumeSurface").run();
    }

    private static void nativeSurfaceChanged(Surface surface, int w, int h) {
        FTMap.cmd("changeSurface").set("w", w).set("h", h).run();
    }

    public static OnMapClickListener MapClickListener;

    public static void setOnMapClickListener(OnMapClickListener listener) {
        MapClickListener = listener;
    }

    public interface OnMapClickListener {
        void Callback(JSONObject dataStr);
    }

    private static void nativeOnTouch(int actionType, int id1, float x1, float y1, int id2, float x2, float y2, int maskedPointer) {
        FTMap.cmd("onTouch")
                .set("actionType", actionType)
                .set("id1", id1)
                .set("x1", x1)
                .set("y1", y1)
                .set("id2", id2)
                .set("x2", x2)
                .set("y2", y2)
                .set("maskedPointer", maskedPointer)
                .run();
      /*  if (actionType == 2) {
            isUp = false;
            if (firstClickTime == 0 & secondClickTime == 0) {//第一次点击
                firstClickTime = System.currentTimeMillis();
                firstClickX = x1;
                firstClickY = y1;
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if (!isUp) {
                            Log.d("OnMapClickListener", "长按");
                            firstClickTime = 0;
                            secondClickTime = 0;
                            isDoubleClick = false;
                            JSONObject jsonObject = new JSONObject();
                            try {
                                jsonObject.put("clickTypr", "longClick");
                                jsonObject.put("screenX", x1);
                                jsonObject.put("screenY", y1);
                                MapClickListener.Callback(jsonObject);
                            } catch (JSONException e) {
                                e.printStackTrace();
                            }

                        } else {
                            if (!isDoubleClick) {
                                Log.d("OnMapClickListener", "点击");
                                JSONObject jsonObject = new JSONObject();
                                try {
                                    jsonObject.put("clickTypr", "Click");
                                    jsonObject.put("screenX", x1);
                                    jsonObject.put("screenY", y1);
                                    MapClickListener.Callback(jsonObject);
                                } catch (JSONException e) {
                                    e.printStackTrace();
                                }
                            }
                            isDoubleClick = false;
                            firstClickTime = 0;
                            secondClickTime = 0;
                        }
                    }
                }, 300);

            } else {
                secondClickTime = System.currentTimeMillis();
                stillTime = secondClickTime - firstClickTime;
                if (stillTime < 300) {//两次点击小于0.3秒

                    JSONObject jsonObject = new JSONObject();
                    try {
                        jsonObject.put("clickTypr", "doubleClick");
                        jsonObject.put("screenX", x1);
                        jsonObject.put("screenY", y1);
                        MapClickListener.Callback(jsonObject);
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
//                    if (firstClickX+5>x1||firstClickX-5>x1){
//                    Log.d("OnMapClickListener","双击");
//                    }else{
//                        Log.d("OnMapClickListener","双点");
//                    }
                    isDoubleClick = true;
                    firstClickTime = 0;
                    secondClickTime = 0;
                }
            }

//            FMap.INSTANCE.PtoG(x1, y1, (JSONObject results) -> {
//                if (results != null) {
//                    String s1 = results.toString();
//                    MapClickListener.Callback(results);
//                    Log.d("PtoG", s1);
//                }
//            });
        } else if (actionType == 1) {
            isUp = true;
        }
*/
        Log.d("nativeOnTouch", "actionType=" + actionType + "\nid1=" + id1 + "\nx1=" + x1 + "\ny1=" + y1 + "\nid2=" + id2 + "\nx2=" + x2 + "\ny2=" + y2);
    }


//  private static native void nativeSetRenderingInitializationFinishedListener(
//      @Nullable MapRenderingListener listener);
}
