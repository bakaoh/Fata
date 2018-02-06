package com.bakaoh.fata;

import android.view.Surface;

/**
 * Created by taitt on 07/02/2018.
 */

public class NativeHelper {

    static {
        System.loadLibrary("native-lib");
    }

    /**
     * This hello world actually won't show the message "hello world" in the terminal 👅
     * Instead we're going to print out information about the video,
     * things like its format (container), duration, resolution, audio channels
     * and, in the end, we'll decode some frames and save them as image files.
     */
    public static native int helloWorld(String inputFile, String outputFolder);

    public static native void renderSurface(String inputFile, Surface surface);

}
