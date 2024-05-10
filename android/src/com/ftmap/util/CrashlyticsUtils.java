package com.ftmap.util;



import android.util.Log;

import androidx.annotation.NonNull;

import com.ftmap.app.MapTestApplication;


public final class CrashlyticsUtils
{
    public static void logException(@NonNull Throwable exception)
    {
        if (!checkCrashlytics())
            return;

//        FirebaseCrashlytics.getInstance().recordException(exception);
    }

    public static void log(int priority, @NonNull String tag, @NonNull String msg)
    {
        if (!checkCrashlytics())
            return;

//        FirebaseCrashlytics.getInstance().log(toLevel(priority) + "/" + tag + ": " + msg);
    }

    private static boolean checkCrashlytics()
    {
        MapTestApplication app = MapTestApplication.get();
        return false;
    }

    @NonNull
    private static String toLevel(int level)
    {
        switch (level)
        {
            case Log.VERBOSE:
                return "V";
            case Log.DEBUG:
                return "D";
            case Log.INFO:
                return "I";
            case Log.WARN:
                return "W";
            case Log.ERROR:
                return "E";
            default:
                throw new IllegalArgumentException("Undetermined log level: " + level);
        }
    }

    private CrashlyticsUtils() {}
}

