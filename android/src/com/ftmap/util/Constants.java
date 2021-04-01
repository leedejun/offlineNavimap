package com.ftmap.util;

public final class Constants
{
  public static final String STORAGE_PATH = "/Android/data/%s/%s/";
  public static final String OBB_PATH = "/Android/obb/%s/";

  public static final int KB = 1024;
  public static final int MB = 1024 * 1024;
  public static final int GB = 1024 * 1024 * 1024;

  static final int CONNECTION_TIMEOUT_MS = 5000;
  static final int READ_TIMEOUT_MS = 30000;

  public static final String MWM_DIR_POSTFIX = "/MapsWithMe/";
  public static final String CACHE_DIR = "cache";
  public static final String FILES_DIR = "files";

  private Constants() {}
}
