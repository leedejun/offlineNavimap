package com.ftmap.maps.search;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.ftmap.base.Initializable;
import com.ftmap.maps.FTMap;
import com.ftmap.maps.Framework;

import com.ftmap.maps.HotelsFilter;
import com.ftmap.maps.api.ParsedMwmRequest;
import com.ftmap.util.Language;
import com.ftmap.util.Listeners;
import com.ftmap.util.concurrency.UiThread;
import com.ftmap.util.log.Logger;
import com.ftmap.util.log.LoggerFactory;
import com.ftmap.maps.FTMap;
import java.io.UnsupportedEncodingException;

public enum SearchEngine implements NativeSearchListener,
                                    NativeMapSearchListener,
                                    NativeBookmarkSearchListener,
                                    Initializable<Void>
{
  INSTANCE;

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SearchEngine.class.getSimpleName();

  // Query, which results are shown on the map.
  @Nullable
  private String mQuery;

  @Override
  public void onResultsUpdate(@NonNull final SearchResult[] results, final long timestamp,
                              final boolean isHotel)
  {
    UiThread.run(
        () ->
        {
          for (NativeSearchListener listener : mListeners)
            listener.onResultsUpdate(results, timestamp, isHotel);
          mListeners.finishIterate();
        });
  }

  @Override
  public void onResultsEnd(final long timestamp)
  {
    UiThread.run(
        () ->
        {
          for (NativeSearchListener listener : mListeners)
            listener.onResultsEnd(timestamp);
          mListeners.finishIterate();
        });
  }

  @Override
  public void onMapSearchResults(final NativeMapSearchListener.Result[] results, final long timestamp, final boolean isLast)
  {
    UiThread.run(
        () ->
        {
          for (NativeMapSearchListener listener : mMapListeners)
            listener.onMapSearchResults(results, timestamp, isLast);
          mMapListeners.finishIterate();
        });
  }

  public void onBookmarkSearchResultsUpdate(@Nullable long[] bookmarkIds, long timestamp)
  {
    for (NativeBookmarkSearchListener listener : mBookmarkListeners)
      listener.onBookmarkSearchResultsUpdate(bookmarkIds, timestamp);
    mBookmarkListeners.finishIterate();
  }

  public void onBookmarkSearchResultsEnd(@Nullable long[] bookmarkIds, long timestamp)
  {
    for (NativeBookmarkSearchListener listener : mBookmarkListeners)
      listener.onBookmarkSearchResultsEnd(bookmarkIds, timestamp);
    mBookmarkListeners.finishIterate();
  }



  @NonNull
  private final Listeners<NativeSearchListener> mListeners = new Listeners<>();
  @NonNull
  private final Listeners<NativeMapSearchListener> mMapListeners = new Listeners<>();
  @NonNull
  private final Listeners<NativeBookmarkSearchListener> mBookmarkListeners = new Listeners<>();


  public void addListener(NativeSearchListener listener)
  {
    mListeners.register(listener);
  }

  public void removeListener(NativeSearchListener listener)
  {
    mListeners.unregister(listener);
  }

  public void addMapListener(NativeMapSearchListener listener)
  {
    mMapListeners.register(listener);
  }

  public void removeMapListener(NativeMapSearchListener listener)
  {
    mMapListeners.unregister(listener);
  }

  public void addBookmarkListener(NativeBookmarkSearchListener listener)
  {
    mBookmarkListeners.register(listener);
  }

  public void removeBookmarkListener(NativeBookmarkSearchListener listener)
  {
    mBookmarkListeners.unregister(listener);
  }



  private native void nativeInit();

  /**
   * @param timestamp Search results are filtered according to it after multiple requests.
   * @return whether search was actually started.
   */
  @MainThread
  public boolean search(String query, long timestamp, boolean hasLocation,
                               double lat, double lon, @Nullable HotelsFilter hotelsFilter,
                               @Nullable BookingFilterParams bookingParams)
  {
    try
    {
      return nativeRunSearch(query.getBytes("utf-8"), Language.getKeyboardLocale(),
                             timestamp, hasLocation, lat, lon, hotelsFilter, bookingParams);
    } catch (UnsupportedEncodingException ignored) { }

//    FTMap.getIns().search("");
    return false;
  }

  @MainThread
  public void searchInteractive(@NonNull String query, @NonNull String locale, long timestamp,
                                       boolean isMapAndTable, @Nullable HotelsFilter hotelsFilter,
                                       @Nullable BookingFilterParams bookingParams)
  {
    try
    {
      nativeRunInteractiveSearch(query.getBytes("utf-8"), locale, timestamp, isMapAndTable,
                                 hotelsFilter, bookingParams);
    } catch (UnsupportedEncodingException ignored) { }
  }

  @MainThread
  public void searchInteractive(@NonNull String query, long timestamp, boolean isMapAndTable,
                                       @Nullable HotelsFilter hotelsFilter, @Nullable BookingFilterParams bookingParams)
  {
    searchInteractive(query, Language.getKeyboardLocale(), timestamp, isMapAndTable, hotelsFilter, bookingParams);
  }

  @MainThread
  public static void searchMaps(String query, long timestamp)
  {
    try
    {
      nativeRunSearchMaps(query.getBytes("utf-8"), Language.getKeyboardLocale(), timestamp);
    } catch (UnsupportedEncodingException ignored) { }
  }

//  @MainThread
//  public boolean searchInBookmarks(@NonNull String query, long categoryId, long timestamp)
//  {
//    try
//    {
//      return nativeRunSearchInBookmarks(query.getBytes("utf-8"), categoryId, timestamp);
//    } catch (UnsupportedEncodingException ex)
//    {
//      LOGGER.w(TAG, "Unsupported encoding in bookmarks search.", ex);
//    }
//    return false;
//  }

  public void setQuery(@Nullable String query)
  {
    mQuery = query;
  }

  @Nullable
  public String getQuery()
  {
    return mQuery;
  }

  @MainThread
  public void cancel()
  {
    cancelApiCall();
    cancelAllSearches();
  }

  @MainThread
  private static void cancelApiCall()
  {
    if (ParsedMwmRequest.hasRequest())
      ParsedMwmRequest.setCurrentRequest(null);
//    FTMap.nativeClearApiPoints();
  }

  @MainThread
  public void cancelInteractiveSearch()
  {
    mQuery = "";
    nativeCancelInteractiveSearch();
  }

  @MainThread
  private void cancelAllSearches()
  {
    mQuery = "";
    nativeCancelAllSearches();
  }

  @MainThread
  public void showResult(int index)
  {
    mQuery = "";
    nativeShowResult(index);
  }

  @Override
  public void initialize(@Nullable Void aVoid)
  {
    nativeInit();
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  /**
   * @param bytes utf-8 formatted bytes of query.
   */
  private static native boolean nativeRunSearch(byte[] bytes, String language, long timestamp, boolean hasLocation,
                                                double lat, double lon, @Nullable HotelsFilter hotelsFilter,
                                                @Nullable BookingFilterParams bookingParams);

  /**
   * @param bytes utf-8 formatted query bytes
   * @param bookingParams
   */
  private static native void nativeRunInteractiveSearch(byte[] bytes, String language, long timestamp,
                                                        boolean isMapAndTable, @Nullable HotelsFilter hotelsFilter,
                                                        @Nullable BookingFilterParams bookingParams);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunSearchMaps(byte[] bytes, String language, long timestamp);

//  private static native boolean nativeRunSearchInBookmarks(byte[] bytes, long categoryId, long timestamp);

  private static native void nativeShowResult(int index);

  private static native void nativeCancelInteractiveSearch();

  private static native void nativeCancelEverywhereSearch();

  private static native void nativeCancelAllSearches();

  /**
   * @return all existing hotel types
   */
  @NonNull
  static native HotelsFilter.HotelType[] nativeGetHotelTypes();
}