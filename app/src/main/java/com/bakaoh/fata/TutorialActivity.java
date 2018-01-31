package com.bakaoh.fata;

import android.content.Context;
import android.content.Intent;
import android.os.Environment;

import org.libsdl.app.SDLActivity;

public class TutorialActivity extends SDLActivity {

    private static final String MAIN_FUNCTION = "main-function";

    public static Intent buildIntent(Context context, String mainFunction) {
        return new Intent(context, TutorialActivity.class)
                .putExtra(MAIN_FUNCTION, mainFunction);
    }

    protected String getMainFunction() {
        return getIntent().getStringExtra(MAIN_FUNCTION);
    }

    protected String[] getArguments() {
        return new String[]{Environment.getExternalStorageDirectory() + "/fata/small_bunny_1080p_60fps.mp4"};
    }
}
