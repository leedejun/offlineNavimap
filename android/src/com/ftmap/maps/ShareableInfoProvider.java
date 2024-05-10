package com.ftmap.maps;

import androidx.annotation.NonNull;

public interface ShareableInfoProvider {
    @NonNull
    String getName();

    double getLat();

    double getLon();

    double getScale();

    @NonNull
    String getAddress();
}
