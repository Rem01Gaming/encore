/*
 * Copyright (C) 2026 Rem01Gaming
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.rem01gaming.encore;

import android.content.Context;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.util.Log;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;

public final class ToastBridge {

    private static final String TAG = "EncoreTweaks";
    private static final String SOCKET_NAME = "encore_toast_bridge";
    private static final int CLIENT_TIMEOUT_MS = 2000;

    private static boolean started;
    private static Handler mainHandler;

    private ToastBridge() {
    }

    public static synchronized void install(ClassLoader loader) {
        if (started) return;
        started = true;

        Thread listener = new Thread(ToastBridge::listenLoop, "EncoreToastBridge-Listener");
        listener.setDaemon(true);
        listener.start();
    }

    private static void listenLoop() {
        try (LocalServerSocket server = new LocalServerSocket(SOCKET_NAME)) {
            Log.i(TAG, "listening for toast requests");
            while (true) {
                try (LocalSocket client = server.accept()) {
                    if (!isTrustedPeer(client)) {
                        Log.w(TAG, "rejected connection from untrusted uid");
                        continue;
                    }
                    client.setSoTimeout(CLIENT_TIMEOUT_MS);
                    handleClient(client);
                } catch (IOException e) {
                    Log.w(TAG, "client I/O error", e);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "listener died", e);
        }
    }

    private static boolean isTrustedPeer(LocalSocket client) {
        try {
            int peerUid = client.getPeerCredentials().getUid();
            return peerUid == Process.ROOT_UID || peerUid == Process.SHELL_UID;
        } catch (IOException e) {
            return false;
        }
    }

    private static void handleClient(LocalSocket client) throws IOException {
        BufferedReader reader = new BufferedReader(
                new InputStreamReader(client.getInputStream(), StandardCharsets.UTF_8));
        String line = reader.readLine();
        if (line == null) return;

        String[] parts = line.split("::", 2);
        if (parts.length != 2) {
            Log.w(TAG, "malformed command: " + line);
            return;
        }

        if (parts[0].equals("TOAST")) {
            showToast(parts[1]);
        } else {
            Log.w(TAG, "unknown command: " + parts[0]);
        }
    }

    private static void showToast(String payload) {
        int duration = Toast.LENGTH_SHORT;
        String message = payload;

        int separator = payload.indexOf("::");
        if (separator > 0) {
            String token = payload.substring(0, separator);
            if (token.equals("SHORT") || token.equals("LONG")) {
                duration = token.equals("LONG") ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT;
                message = payload.substring(separator + 2);
            }
        }
        if (message.isEmpty()) return;

        Context context = currentContext();
        Handler handler = getMainHandler();
        if (context == null || handler == null) {
            Log.w(TAG, "toast dropped, SystemUI is not ready yet");
            return;
        }

        final int finalDuration = duration;
        final String finalMessage = message;
        handler.post(() -> {
            try {
                Toast.makeText(context, finalMessage, finalDuration).show();
            } catch (Throwable t) {
                Log.e(TAG, "failed to show toast", t);
            }
        });
    }

    private static Context currentContext() {
        try {
            Class<?> activityThread = Class.forName("android.app.ActivityThread");
            Method currentApplication = activityThread.getDeclaredMethod("currentApplication");
            return (Context) currentApplication.invoke(null);
        } catch (ReflectiveOperationException | RuntimeException e) {
            Log.w(TAG, "failed to resolve application context", e);
            return null;
        }
    }

    private static synchronized Handler getMainHandler() {
        if (mainHandler == null) {
            Looper mainLooper = Looper.getMainLooper();
            if (mainLooper == null) return null;
            mainHandler = new Handler(mainLooper);
        }
        return mainHandler;
    }
}
