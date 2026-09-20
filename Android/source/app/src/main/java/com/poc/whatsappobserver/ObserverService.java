package com.poc.whatsappobserver;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Environment;
import android.os.FileObserver;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ObserverService extends Service implements RecursiveFileObserver.EventListener {
    private static final String TAG = ObserverService.class.getSimpleName();

    private static Map<Integer, String> eventNames = new HashMap<>();
    static {
        eventNames.put(FileObserver.ACCESS, "ACCESS");
        eventNames.put(FileObserver.ATTRIB, "ATTRIB");
        eventNames.put(FileObserver.CLOSE_NOWRITE, "CLOSE_NOWRITE");
        eventNames.put(FileObserver.CLOSE_WRITE, "CLOSE_WRITE");
        eventNames.put(FileObserver.CREATE, "CREATE");
        eventNames.put(FileObserver.DELETE, "DELETE");
        eventNames.put(FileObserver.DELETE_SELF, "DELETE_SELF");
        eventNames.put(FileObserver.MODIFY, "MODIFY");
        eventNames.put(FileObserver.MOVED_FROM, "MOVED_FROM");
        eventNames.put(FileObserver.MOVED_TO, "MOVED_TO");
        eventNames.put(FileObserver.MOVE_SELF, "MOVE_SELF");
        eventNames.put(FileObserver.OPEN, "OPEN");
    }

    private List<RecursiveFileObserver> observers = new ArrayList<>();

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        List<String> paths = new ArrayList<>();
        paths.add(new File(getExternalMediaDirs()[0].getParentFile(), "com.whatsapp/WhatsApp").getAbsolutePath());

        for (String path : paths) {
            RecursiveFileObserver observer = new RecursiveFileObserver(path, FileObserver.ALL_EVENTS, this);
            observer.startWatching();
            LogCache.d(TAG, "NEW Observing directory: " + path);
            observers.add(observer);
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                    }
                    LogCache.d(TAG, "Still running");
                }
            }
        }).start();
    }

    private void startForeground() {
        try {
            NotificationChannel channel = new NotificationChannel("CHANNEL_ID", "My Channel", NotificationManager.IMPORTANCE_DEFAULT);
            channel.setDescription("Channel for foreground service notification");

            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);

            Notification notification = new NotificationCompat.Builder(this, "CHANNEL_ID")
                    .setContentTitle("Observing Files")
                    .setAutoCancel(false)
                            .build();
            ServiceCompat.startForeground(
                    this,
                    100,
                    notification,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE
            );
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        LogCache.d(TAG, "onStartCommand");
        startForeground();
        return super.onStartCommand(intent, flags, startId);
    }

    public static void launch(Context context) {
        Intent intent = new Intent();
        intent.setClass(context, ObserverService.class);
        context.startForegroundService(intent);
    }

    private String eventToString(int event) {
        return eventNames.get(event);
    }

    @Override
    public void onEvent(int event, File file) {
        LogCache.d(TAG, "File event " + eventToString(event) + ": " + file.getAbsolutePath());

        if (file.isDirectory()) return;
    }
}
