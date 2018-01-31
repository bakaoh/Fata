package com.bakaoh.fata;

import android.os.Environment;

import org.libsdl.app.SDLActivity;

public class Tutorial02Activity extends SDLActivity {

    protected String getMainFunction() {
        return "tutorial02";
    }

    protected String[] getArguments() {
        return new String[]{Environment.getExternalStorageDirectory() + "/fata/small_bunny_1080p_60fps.mp4"};
    }
}
