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

import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.os.Process;
import android.util.Log;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;

public final class BinderResolver {

    private static final String TAG = "EncoreTweaks";
    private static final String SOCKET_NAME = "encore_binder_resolver";

    private static boolean started;

    private BinderResolver() {
    }

    public static synchronized void start() {
        if (started) return;
        started = true;

        Thread listener = new Thread(BinderResolver::listenLoop, "EncoreBinderResolver-Listener");
        listener.setDaemon(true);
        listener.start();
    }

    private static void listenLoop() {
        try (LocalServerSocket server = new LocalServerSocket(SOCKET_NAME)) {
            Log.i(TAG, "listening for binder field requests");
            while (true) {
                try (LocalSocket client = server.accept()) {
                    if (!isTrustedPeer(client)) {
                        Log.w(TAG, "rejected connection from untrusted uid");
                        continue;
                    }
                    handleClient(client);
                } catch (IOException e) {
                    Log.w(TAG, "client I/O error", e);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "listener stopped", e);
        }
    }

    private static boolean isTrustedPeer(LocalSocket client) {
        try {
            return client.getPeerCredentials().getUid() == Process.ROOT_UID;
        } catch (IOException e) {
            return false;
        }
    }

    private static void handleClient(LocalSocket client) throws IOException {
        BufferedReader reader = new BufferedReader(
                new InputStreamReader(client.getInputStream(), StandardCharsets.UTF_8));
        BufferedWriter writer = new BufferedWriter(
                new OutputStreamWriter(client.getOutputStream(), StandardCharsets.UTF_8));

        String query;
        while ((query = reader.readLine()) != null) {
            if (query.isEmpty()) continue;

            String reply = resolve(query);
            writer.write(reply);
            writer.newLine();
            writer.flush();
        }
    }

    private static String resolve(String query) {
        int separator = query.indexOf("::");
        if (separator < 0) {
            return "ERROR malformed query, expected Class::FIELD";
        }

        String className = query.substring(0, separator);
        int stubPosition = className.indexOf(".Stub");
        if (stubPosition >= 0) {
            className = className.substring(0, stubPosition) + "$Stub"
                    + className.substring(stubPosition + 5);
        }
        String fieldName = query.substring(separator + 2);
        if (className.isEmpty() || fieldName.isEmpty()) {
            return "ERROR malformed query, expected Class::FIELD";
        }

        try {
            Class<?> targetClass = Class.forName(className, false, BinderResolver.class.getClassLoader());
            Field field = targetClass.getDeclaredField(fieldName);
            if (field.getType() != int.class || !Modifier.isStatic(field.getModifiers())) {
                return "ERROR field is not a static int: " + fieldName;
            }
            field.setAccessible(true);
            return Integer.toString(field.getInt(null));
        } catch (ClassNotFoundException | LinkageError e) {
            return "ERROR class not found: " + className.replace('.', '/');
        } catch (NoSuchFieldException e) {
            return "ERROR field not found: " + fieldName;
        } catch (IllegalAccessException | SecurityException e) {
            Log.w(TAG, "failed to resolve " + query, e);
            return "ERROR unable to access field: " + fieldName;
        }
    }
}
