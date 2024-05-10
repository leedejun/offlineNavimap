package com.ftmap.maps;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;


import com.ftmap.app.MapTestActivity;
import com.ftmap.util.UiUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class Notifier
{
    private static final String EXTRA_CANCEL_NOTIFICATION = "extra_cancel_notification";
    private static final String EXTRA_NOTIFICATION_CLICKED = "extra_notification_clicked";

    public static final int ID_NONE = 0;
    public static final int ID_DOWNLOAD_FAILED = 1;
    public static final int ID_IS_NOT_AUTHENTICATED = 2;
    public static final int ID_LEAVE_REVIEW = 3;
    @NonNull
    private final Application mContext;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ ID_NONE, ID_DOWNLOAD_FAILED, ID_IS_NOT_AUTHENTICATED, ID_LEAVE_REVIEW })
    public @interface NotificationId
    {
    }

    private Notifier(@NonNull Application context)
    {
        mContext = context;
    }

    public void notifyDownloadFailed(@Nullable String id, @Nullable String name)
    {
//        String title = mContext.getString(R.string.app_name);
//        String content = mContext.getString(R.string.download_country_failed, name);
//
//        Intent intent = MapTestActivity.createShowMapIntent(mContext, id)
//                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//        PendingIntent pi = PendingIntent.getActivity(mContext, 0, intent,
//                PendingIntent.FLAG_UPDATE_CURRENT);
//
//        String channel = NotificationChannelFactory.createProvider(mContext).getDownloadingChannel();
//        placeNotification(title, content, pi, ID_DOWNLOAD_FAILED, channel);
//        Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN);
    }

    void notifyAuthentication()
    {
//        Intent authIntent = MapTestActivity.createAuthenticateIntent(mContext);
//        authIntent.putExtra(EXTRA_CANCEL_NOTIFICATION, Notifier.ID_IS_NOT_AUTHENTICATED);
//        authIntent.putExtra(EXTRA_NOTIFICATION_CLICKED,
//                Statistics.EventName.UGC_NOT_AUTH_NOTIFICATION_CLICKED);
//
//        PendingIntent pi = PendingIntent.getActivity(mContext, 0, authIntent,
//                PendingIntent.FLAG_UPDATE_CURRENT);
//
//        String channel = NotificationChannelFactory.createProvider(mContext).getUGCChannel();
//        NotificationCompat.Builder builder =
//                getBuilder(mContext.getString(R.string.notification_unsent_reviews_title),
//                        mContext.getString(R.string.notification_unsent_reviews_message),
//                        pi, channel);
//
//        builder.addAction(0, mContext.getString(R.string.authorization_button_sign_in), pi);
//
//        getNotificationManager().notify(ID_IS_NOT_AUTHENTICATED, builder.build());
//
//        Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_NOT_AUTH_NOTIFICATION_SHOWN);
    }

    void notifyLeaveReview(@NonNull NotificationCandidate.UgcReview source)
    {
//        Intent reviewIntent = MapTestActivity.createLeaveReviewIntent(mContext, source);
//        reviewIntent.putExtra(EXTRA_CANCEL_NOTIFICATION, Notifier.ID_LEAVE_REVIEW);
//        reviewIntent.putExtra(EXTRA_NOTIFICATION_CLICKED,
//                Statistics.EventName.UGC_REVIEW_NOTIFICATION_CLICKED);
//
//        PendingIntent pi =
//                PendingIntent.getActivity(mContext, 0, reviewIntent, PendingIntent.FLAG_UPDATE_CURRENT);
//
//        String channel = NotificationChannelFactory.createProvider(mContext).getUGCChannel();
//        String content = source.getAddress().isEmpty()
//                ? source.getReadableName()
//                : source.getReadableName() + ", " + source.getAddress();
//
//        NotificationCompat.Builder builder =
//                getBuilder(mContext.getString(R.string.notification_leave_review_v2_android_short_title),
//                        content, pi, channel)
//                        .setStyle(new NotificationCompat.BigTextStyle()
//                                .setBigContentTitle(
//                                        mContext.getString(R.string.notification_leave_review_v2_title))
//                                .bigText(content))
//                        .addAction(0, mContext.getString(R.string.leave_a_review), pi);
//
//        getNotificationManager().notify(ID_LEAVE_REVIEW, builder.build());
//
//        Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_REVIEW_NOTIFICATION_SHOWN);
    }

    public void cancelNotification(@NotificationId int id)
    {
        if (id == ID_NONE)
            return;

        getNotificationManager().cancel(id);
    }

    public void processNotificationExtras(@Nullable Intent intent)
    {
        if (intent == null)
            return;

        if (intent.hasExtra(Notifier.EXTRA_CANCEL_NOTIFICATION))
        {
            @Notifier.NotificationId
            int notificationId = intent.getIntExtra(Notifier.EXTRA_CANCEL_NOTIFICATION, Notifier.ID_NONE);
            cancelNotification(notificationId);
        }

        if (intent.hasExtra(Notifier.EXTRA_NOTIFICATION_CLICKED))
        {
            String eventName = intent.getStringExtra(Notifier.EXTRA_NOTIFICATION_CLICKED);
//            Statistics.INSTANCE.trackEvent(eventName);
        }
    }

    private void placeNotification(String title, String content, PendingIntent pendingIntent,
                                   int notificationId, @NonNull String channel)
    {
        final Notification notification = getBuilder(title, content, pendingIntent, channel).build();

        getNotificationManager().notify(notificationId, notification);
    }

    @NonNull
    private NotificationCompat.Builder getBuilder(String title, String content,
                                                  PendingIntent pendingIntent, @NonNull String channel)
    {
        return null;
//        return new NotificationCompat.Builder(mContext, channel)
//                .setAutoCancel(true)
//                .setSmallIcon(R.drawable.pw_notification)
//                .setColor(UiUtils.getNotificationColor(mContext))
//                .setContentTitle(title)
//                .setContentText(content)
//                .setTicker(getTicker(title, content))
//                .setContentIntent(pendingIntent);
    }

//    @NonNull
//    private CharSequence getTicker(String title, String content)
//    {
//        int templateResId = StringUtils.isRtl() ? R.string.notification_ticker_rtl
//                : R.string.notification_ticker_ltr;
//        return mContext.getString(templateResId, title, content);
//    }

    @SuppressWarnings("ConstantConditions")
    @NonNull
    private NotificationManager getNotificationManager()
    {
        return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    @NonNull
    public static Notifier from(Application application)
    {
        return new Notifier(application);
    }
}