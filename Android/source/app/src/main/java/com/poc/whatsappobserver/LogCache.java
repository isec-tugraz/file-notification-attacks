package com.poc.whatsappobserver;

import android.util.Log;

import java.text.SimpleDateFormat;
import java.util.ArrayDeque;
import java.util.Date;
import java.util.Deque;
import java.util.Locale;

public class LogCache {
    private static final int MAX_LINES = 1000;

    private static final SimpleDateFormat FORMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US);
    private static final Deque<String> lines = new ArrayDeque<>();

    public static void d(String tag, String message) {
        Log.d(tag, message);

        synchronized (lines) {
            lines.addLast(FORMAT.format(new Date()) + " " + tag + ": " + message);
            while (lines.size() > MAX_LINES) {
                lines.removeFirst();
            }
        }
    }

    public static String getText() {
        StringBuilder builder = new StringBuilder();
        synchronized (lines) {
            for (String line : lines) {
                builder.append(line).append('\n');
            }
        }
        return builder.toString();
    }
}
