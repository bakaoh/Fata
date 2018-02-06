package com.bakaoh.fata;

import android.app.DownloadManager;
import android.content.Context;
import android.net.Uri;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private String downloadUrl = "http://128.199.206.130/file/small_bunny_1080p_60fps.mp4";
    private String fileName = "bunny_1080p_60fps.mp4";
    private String videoFile = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS) + "/" + fileName;

    private Button btnDownload,
            btnTutorial02, btnTutorial03, btnTutorial04,
            btnTutorial05, btnTutorial06, btnTutorial07,
            btnTest01;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnDownload = findViewById(R.id.download_btn);
        btnDownload.setOnClickListener(this);
        btnTutorial02 = findViewById(R.id.tutorial02_btn);
        btnTutorial02.setOnClickListener(this);
        btnTutorial03 = findViewById(R.id.tutorial03_btn);
        btnTutorial03.setOnClickListener(this);
        btnTutorial04 = findViewById(R.id.tutorial04_btn);
        btnTutorial04.setOnClickListener(this);
        btnTutorial05 = findViewById(R.id.tutorial05_btn);
        btnTutorial05.setOnClickListener(this);
        btnTutorial06 = findViewById(R.id.tutorial06_btn);
        btnTutorial06.setOnClickListener(this);
        btnTutorial07 = findViewById(R.id.tutorial07_btn);
        btnTutorial07.setOnClickListener(this);
        btnTest01 = findViewById(R.id.test01_btn);
        btnTest01.setOnClickListener(this);
    }

    private void download(String url, String outputFile) {
        DownloadManager.Request request = new DownloadManager.Request(Uri.parse(url));
        request.setDescription("Fata: Downloading test video");
        request.setTitle(outputFile);
        request.allowScanningByMediaScanner();
        request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        request.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS, outputFile);

        // get download service and enqueue file
        DownloadManager manager = (DownloadManager) getSystemService(Context.DOWNLOAD_SERVICE);
        manager.enqueue(request);
    }

    @Override
    public void onClick(View view) {
        File file = new File(videoFile);
        if (view == btnDownload) {
            if (file.exists()) {
                Toast.makeText(this, "Test video downloaded", Toast.LENGTH_LONG).show();
            } else {
                download(downloadUrl, fileName);
            }
        } else if (!file.exists()) {
            Toast.makeText(this, "Test video not found", Toast.LENGTH_LONG).show();
        } else if (view == btnTutorial02) {
            startActivity(TutorialActivity.buildIntent(this, "tutorial02", videoFile));
        } else if (view == btnTutorial03) {
            startActivity(TutorialActivity.buildIntent(this, "tutorial03", videoFile));
        } else if (view == btnTutorial04) {
            startActivity(TutorialActivity.buildIntent(this, "tutorial04", videoFile));
        } else if (view == btnTutorial05) {
            startActivity(TutorialActivity.buildIntent(this, "tutorial05", videoFile));
        } else if (view == btnTutorial06) {
            startActivity(TutorialActivity.buildIntent(this, "tutorial06", videoFile));
        } else if (view == btnTutorial07) {
            startActivity(TutorialActivity.buildIntent(this, "tutorial07", videoFile));
        } else if (view == btnTest01) {
            startActivity(NativeRenderActivity.buildIntent(this, videoFile));
        }
    }
}
