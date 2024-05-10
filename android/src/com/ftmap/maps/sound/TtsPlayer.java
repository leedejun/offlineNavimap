package com.ftmap.maps.sound;

import android.content.Context;
import android.content.res.Resources;
import android.speech.tts.TextToSpeech;
import android.text.TextUtils;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.ftmap.app.MapTestActivity;
import com.ftmap.app.MapTestApplication;
import com.ftmap.base.Initializable;
import com.ftmap.base.MediaPlayerWrapper;
import com.ftmap.maps.FTMap;
import com.ftmap.maps.R;
import com.ftmap.util.Config;
import com.ftmap.util.log.Logger;
import com.ftmap.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * {@code TtsPlayer} class manages available TTS voice languages.
 * Single TTS language is described by {@link LanguageData} item.
 * <p>
 * We support a set of languages listed in {@code strings-tts.xml} file.
 * During loading each item in this list is marked as {@code downloaded} or {@code not downloaded},
 * unsupported voices are excluded.
 * <p>
 * At startup we check whether currently selected language is in our list of supported voices and its data is downloaded.
 * If not, we check system default locale. If failed, the same check is made for English language.
 * Finally, if mentioned checks fail we manually disable TTS, so the user must go to the settings and select
 * preferred voice language by hand.
 * <p>
 * If no core supported languages can be used by the system, TTS is locked down and can not be enabled and used.
 */
public enum TtsPlayer implements Initializable<Context>
{
  INSTANCE;

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = TtsPlayer.class.getSimpleName();
  private static final Locale DEFAULT_LOCALE = Locale.SIMPLIFIED_CHINESE;
  private static final float SPEECH_RATE = 1.2f;

  private TextToSpeech mTts;
  private boolean mInitializing;

  // TTS is locked down due to absence of supported languages
  private boolean mUnavailable;

  TtsPlayer() {}

  private static void reportFailure(IllegalArgumentException e, String location)
  {
//    Statistics.INSTANCE.trackEvent(Statistics.EventName.TTS_FAILURE_LOCATION,
//                                   Statistics.params().add(Statistics.EventParam.ERR_MSG, e.getMessage())
//                                                      .add(Statistics.EventParam.FROM, location));
  }

  private static @Nullable LanguageData findSupportedLanguage(String internalCode, List<LanguageData> langs)
  {
    if (TextUtils.isEmpty(internalCode))
      return null;

    for (LanguageData lang : langs)
      if (lang.matchesInternalCode(internalCode))
        return lang;

    return null;
  }

  private static @Nullable LanguageData findSupportedLanguage(Locale locale, List<LanguageData> langs)
  {
    if (locale == null)
      return null;

    for (LanguageData lang : langs)
      if (lang.matchesLocale(locale))
        return lang;

    return null;
  }

  private boolean setLanguageInternal(LanguageData lang)
  {
    try
    {
//       Locale locale = new Locale("en", "US");
      mTts.setLanguage(lang.locale);
      nativeSetTurnNotificationsLocale(lang.internalCode);
      Config.setTtsLanguage(lang.internalCode);

      return true;
    }
    catch (IllegalArgumentException e)
    {
      reportFailure(e, "setLanguageInternal(): " + lang.locale);
      lockDown();
      return false;
    }
  }

  public boolean setLanguage(LanguageData lang)
  {
    return (lang != null && setLanguageInternal(lang));
  }

  private static @Nullable LanguageData getDefaultLanguage(List<LanguageData> langs)
  {
    LanguageData res;

    Locale defLocale = Locale.getDefault();
    if (defLocale != null)
    {
      res = findSupportedLanguage(defLocale, langs);
      if (res != null && res.downloaded)
        return res;
    }

    res = findSupportedLanguage(DEFAULT_LOCALE, langs);
    if (res != null && res.downloaded)
      return res;

    return null;
  }

  public static @Nullable LanguageData getSelectedLanguage(List<LanguageData> langs)
  {
    return findSupportedLanguage(Config.getTtsLanguage(), langs);
  }

  private void lockDown()
  {
    mUnavailable = true;
    setEnabled(false);
  }

  @Override
  public void initialize(@Nullable Context context)
  {
    if (mTts != null || mInitializing || mUnavailable)
      return;

    mInitializing = true;
    mTts = new TextToSpeech(context, new TextToSpeech.OnInitListener()
    {
      @Override
      public void onInit(int status)
      {
        if (status == TextToSpeech.ERROR)
        {
          LOGGER.e(TAG, "Failed to initialize TextToSpeach");
          lockDown();
          mInitializing = false;
          return;
        }

        refreshLanguages(context);
//        int result = mTts.setLanguage(Locale.CHINA);
//        if (result != mTts.LANG_COUNTRY_AVAILABLE
//                && result != mTts.LANG_AVAILABLE) {
//          Toast.makeText(context, "TTS暂时不支持这种语音的朗读！",
//                  Toast.LENGTH_SHORT).show();
//        }

        mTts.setSpeechRate(SPEECH_RATE);
        mInitializing = false;
      }
    });
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  public boolean isSpeaking()
  {
    return mTts != null && mTts.isSpeaking();
  }

  private static boolean isReady()
  {
    return (INSTANCE.mTts != null && !INSTANCE.mUnavailable && !INSTANCE.mInitializing);
  }

  private void speak(String textToSpeak)
  {
    if (Config.isTtsEnabled())
      try
      {
        //noinspection deprecation
        mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, null);
      }
      catch (IllegalArgumentException e)
      {
        reportFailure(e, "speak()");
        lockDown();
      }
  }

  public void playTurnNotifications(@NonNull MediaPlayerWrapper context)
  {
    if (context.isPlaying())
      return;

//    speak("飞度科技");

    // It's necessary to call Framework.nativeGenerateTurnNotifications() even if TtsPlayer is invalid.
    final String[] turnNotifications = FTMap.nativeGenerateNotifications();

    if (turnNotifications != null && isReady())
      for (String textToSpeak : turnNotifications)
        speak(textToSpeak);
  }

  public void stop()
  {
    if (isReady())
      try
      {
        mTts.stop();
      }
      catch (IllegalArgumentException e)
      {
        reportFailure(e, "stop()");
        lockDown();
      }
  }

  public static boolean isEnabled()
  {
    return (isReady() && nativeAreTurnNotificationsEnabled());
  }

  public static void setEnabled(boolean enabled)
  {
    Config.setTtsEnabled(enabled);
    nativeEnableTurnNotifications(enabled);
  }

  private boolean getUsableLanguages(List<LanguageData> outList,Context context)
  {
    Resources resources = context.getResources();
    String[] codes = resources.getStringArray(R.array.tts_languages_supported);
    String[] names = resources.getStringArray(R.array.tts_language_names);

    for (int i = 0; i < codes.length; i++)
    {
      try
      {
        outList.add(new LanguageData(codes[i], names[i], mTts));
      }
      catch (LanguageData.NotAvailableException ignored) {
        LOGGER.e(TAG, "Failed to get usable languages " + ignored.getMessage());
      }
      catch (IllegalArgumentException e)
      {
        LOGGER.e(TAG, "Failed to get usable languages", e);
        reportFailure(e, "getUsableLanguages()");
        lockDown();
        return false;
      }
    }

    return true;
  }

  private @Nullable LanguageData refreshLanguagesInternal(List<LanguageData> outList,Context context)
  {
    if (!getUsableLanguages(outList,context))
      return null;

    if (outList.isEmpty())
    {
      // No supported languages found, lock down TTS :(
      lockDown();
      return null;
    }

    LanguageData res = getSelectedLanguage(outList);
    if (res == null || !res.downloaded)
      // Selected locale is not available or not downloaded
      res = getDefaultLanguage(outList);

    if (res == null || !res.downloaded)
    {
      // Default locale can not be used too
      Config.setTtsEnabled(false);
      return null;
    }

    return res;
  }

  public @NonNull List<LanguageData> refreshLanguages(Context context)
  {
    List<LanguageData> res = new ArrayList<>();
    if (mUnavailable || mTts == null)
      return res;

    LanguageData lang = refreshLanguagesInternal(res,context);
    setLanguage(lang);

    setEnabled(Config.isTtsEnabled());
    return res;
  }

  public native static void nativeEnableTurnNotifications(boolean enable);
  public native static boolean nativeAreTurnNotificationsEnabled();
  public native static void nativeSetTurnNotificationsLocale(String code);
  public native static String nativeGetTurnNotificationsLocale();
}
