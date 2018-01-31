package com.bakaoh.fata;

import android.app.DownloadManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button btnDownload = findViewById(R.id.download_btn);
        btnDownload.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String url = "http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4";
                String outputFile = "bunny_1080p_60fps.mp4";
                download(url, outputFile);
            }
        });
        Button btnTutorial02 = findViewById(R.id.tutorial02_btn);
        btnTutorial02.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startActivity(new Intent(MainActivity.this, Tutorial02Activity.class));
            }
        });
        
//        TextView tv = findViewById(R.id.sample_text);
//        String input = Environment.getExternalStorageDirectory() + "/fata/small_bunny_1080p_60fps.mp4";
//        String output = Environment.getExternalStorageDirectory() + "/fata";
//        tv.setText("Result: " + chapter0(input, output));
    }

    private void download(String url, String outputFile) {
        DownloadManager.Request request = new DownloadManager.Request(Uri.parse(url));
        request.setDescription("Fata: Downloading test video");
        request.setTitle(outputFile);
        // in order for this if to run, you must use the android 3.2 to compile your app
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            request.allowScanningByMediaScanner();
            request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        }
        request.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS, outputFile);

        // get download service and enqueue file
        DownloadManager manager = (DownloadManager) getSystemService(Context.DOWNLOAD_SERVICE);
        manager.enqueue(request);
    }

    /**
     * This hello world actually won't show the message "hello world" in the terminal 👅
     * Instead we're going to print out information about the video,
     * things like its format (container), duration, resolution, audio channels
     * and, in the end, we'll decode some frames and save them as image files.
     */
    public native int chapter0(String inputFile, String outputFolder);
}
