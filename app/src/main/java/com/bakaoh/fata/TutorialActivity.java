package com.bakaoh.fata;

import android.content.Context;
import android.content.Intent;
import android.os.Environment;

import org.libsdl.app.SDLActivity;

public class TutorialActivity extends SDLActivity {

    private static final String MAIN_FUNCTION = "main-function";
    private static final String VIDEO_FILE = "video-file";

    public static Intent buildIntent(Context context, String mainFunction, String videoFile) {
        return new Intent(context, TutorialActivity.class)
                .putExtra(MAIN_FUNCTION, mainFunction)
                .putExtra(VIDEO_FILE, videoFile);
    }

    protected String getMainFunction() {
        return getIntent().getStringExtra(MAIN_FUNCTION);
    }

    protected String[] getArguments() {
        return new String[]{getIntent().getStringExtra(VIDEO_FILE)};
    }
}
