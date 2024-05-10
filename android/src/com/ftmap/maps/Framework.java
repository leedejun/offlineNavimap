package com.ftmap.maps;

import android.graphics.Bitmap;
import android.location.Location;
import android.text.TextUtils;

import androidx.annotation.IntDef;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Size;
import androidx.annotation.UiThread;

import com.ftmap.util.Constants;
import com.ftmap.util.KeyValue;
import com.ftmap.util.log.Logger;
import com.ftmap.util.log.LoggerFactory;

import com.ftmap.maps.location.LocationHelper;


import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.Map;

/**
 * This class wraps android::Framework.cpp class
 * via static methods
 */
public class Framework
{
    private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
    private static final String TAG = Framework.class.getSimpleName();

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({MAP_STYLE_CLEAR, MAP_STYLE_DARK, MAP_STYLE_VEHICLE_CLEAR, MAP_STYLE_VEHICLE_DARK})

    public @interface MapStyle {}

    public static final int MAP_STYLE_CLEAR = 0;
    public static final int MAP_STYLE_DARK = 1;
    public static final int MAP_STYLE_VEHICLE_CLEAR = 3;
    public static final int MAP_STYLE_VEHICLE_DARK = 4;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ ROUTER_TYPE_VEHICLE, ROUTER_TYPE_PEDESTRIAN, ROUTER_TYPE_BICYCLE, ROUTER_TYPE_TAXI,
            ROUTER_TYPE_TRANSIT })

    public @interface RouterType {}

    public static final int ROUTER_TYPE_VEHICLE = 0;
    public static final int ROUTER_TYPE_PEDESTRIAN = 1;
    public static final int ROUTER_TYPE_BICYCLE = 2;
    public static final int ROUTER_TYPE_TAXI = 3;
    public static final int ROUTER_TYPE_TRANSIT = 4;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({DO_AFTER_UPDATE_NOTHING, DO_AFTER_UPDATE_AUTO_UPDATE, DO_AFTER_UPDATE_ASK_FOR_UPDATE})
    public @interface DoAfterUpdate {}

    public static final int DO_AFTER_UPDATE_NOTHING = 0;
    public static final int DO_AFTER_UPDATE_AUTO_UPDATE = 1;
    public static final int DO_AFTER_UPDATE_ASK_FOR_UPDATE = 2;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ROUTE_REBUILD_AFTER_POINTS_LOADING})
    public @interface RouteRecommendationType {}

    public static final int ROUTE_REBUILD_AFTER_POINTS_LOADING = 0;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ SOCIAL_TOKEN_INVALID, SOCIAL_TOKEN_FACEBOOK, SOCIAL_TOKEN_GOOGLE,
            SOCIAL_TOKEN_PHONE, TOKEN_MAPSME })
    public @interface AuthTokenType
    {}
    public static final int SOCIAL_TOKEN_INVALID = -1;
    public static final int SOCIAL_TOKEN_FACEBOOK = 0;
    public static final int SOCIAL_TOKEN_GOOGLE = 1;
    public static final int SOCIAL_TOKEN_PHONE = 2;
    //TODO(@alexzatsepin): remove TOKEN_MAPSME from this list.
    public static final int TOKEN_MAPSME = 3;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ PURCHASE_VERIFIED, PURCHASE_NOT_VERIFIED,
            PURCHASE_VALIDATION_SERVER_ERROR, PURCHASE_VALIDATION_AUTH_ERROR })
    public @interface PurchaseValidationCode {}

    public static final int PURCHASE_VERIFIED = 0;
    public static final int PURCHASE_NOT_VERIFIED = 1;
    public static final int PURCHASE_VALIDATION_SERVER_ERROR = 2;
    public static final int PURCHASE_VALIDATION_AUTH_ERROR = 3;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ SUBSCRIPTION_TYPE_REMOVE_ADS, SUBSCRIPTION_TYPE_BOOKMARK_CATALOG })
    public @interface SubscriptionType {}
    public static final int SUBSCRIPTION_TYPE_REMOVE_ADS = 0;
    public static final int SUBSCRIPTION_TYPE_BOOKMARK_CATALOG = 1;

    @SuppressWarnings("unused")
    public interface PlacePageActivationListener
    {
        void onPlacePageActivated(@NonNull Object data);

        void onPlacePageDeactivated(boolean switchFullScreenMode);
    }

    @SuppressWarnings("unused")
    public interface RoutingListener
    {
        @MainThread
        void onRoutingEvent(int resultCode, String[] missingMaps);
    }

    @SuppressWarnings("unused")
    public interface RoutingProgressListener
    {
        @MainThread
        void onRouteBuildingProgress(float progress);
    }

    @SuppressWarnings("unused")
    public interface RoutingRecommendationListener
    {
        void onRecommend(@RouteRecommendationType int recommendation);
    }

    @SuppressWarnings("unused")
    public interface RoutingLoadPointsListener
    {
        void onRoutePointsLoaded(boolean success);
    }

    @SuppressWarnings("unused")
    public interface PurchaseValidationListener
    {
        void onValidatePurchase(@PurchaseValidationCode int code, @NonNull String serverId,
                                @NonNull String vendorId, @NonNull String encodedPurchaseData,
                                boolean isTrial);
    }

    @SuppressWarnings("unused")
    public interface StartTransactionListener
    {
        void onStartTransaction(boolean success, @NonNull String serverId, @NonNull String vendorId);
    }

    public static class Params3dMode
    {
        public boolean enabled;
        public boolean buildings;
    }

    public static class RouteAltitudeLimits
    {
        public int minRouteAltitude;
        public int maxRouteAltitude;
        public boolean isMetricUnits;
    }

    // this class is just bridge between Java and C++ worlds, we must not create it
    private Framework() {}


    public enum LocalAdsEventType
    {
        LOCAL_ADS_EVENT_SHOW_POINT,
        LOCAL_ADS_EVENT_OPEN_INFO,
        LOCAL_ADS_EVENT_CLICKED_PHONE,
        LOCAL_ADS_EVENT_CLICKED_WEBSITE,
        LOCAL_ADS_EVENT_VISIT
    }
}
