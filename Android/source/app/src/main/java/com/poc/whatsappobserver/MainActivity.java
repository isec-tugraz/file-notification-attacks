package com.poc.whatsappobserver;

import android.os.Bundle;
import android.os.Environment;
import android.os.FileObserver;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
    private TextView logView;
    private Button refreshButton;
    private boolean running = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);
        logView = findViewById(R.id.logView);

        ObserverService.launch(this);

        refreshButton = findViewById(R.id.refresh);
        refreshButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        refreshLogText();
                    }
                }
        );
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshLogText();
    }

    private void refreshLogText() {
        logView.setText(LogCache.getText());
    }
}