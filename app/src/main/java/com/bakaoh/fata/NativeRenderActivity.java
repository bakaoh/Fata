package com.bakaoh.fata;

import android.content.Context;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class NativeRenderActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private static final String VIDEO_FILE = "video-file";
    private SurfaceView surfaceView;

    public static Intent buildIntent(Context context, String videoFile) {
        return new Intent(context, NativeRenderActivity.class)
                .putExtra(VIDEO_FILE, videoFile);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_native_render);

        surfaceView = findViewById(R.id.surfaceView);
        surfaceView.getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(final SurfaceHolder surfaceHolder) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                NativeHelper.renderSurface(
                        getIntent().getStringExtra(VIDEO_FILE),
                        surfaceHolder.getSurface());
            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }
}
