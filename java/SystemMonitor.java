/*
 * Copyright (C) 2024-2026 Rem01Gaming
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

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.os.Build;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;

public class SystemMonitor {
    private static final String OUTPUT_PATH = "/data/adb/.config/encore/system_status";
    private static final long POLL_INTERVAL_MS = 500;
    private static final String UNKNOWN_APP = "unknown 0 0";
    private static final String NONE_APP = "none 0 0";

    private static final String[] FOREGROUND_METHOD_CANDIDATES = {
            "getFocusedRootTask",
            "getFocusedStackInfo",
            "getFocusedTaskInfo",
            "getFocusedRootTaskInfo",
            "getTasks",
            "getRunningTasks",
            "getTopActivity"
    };

    private static final String[] COMPONENT_NAME_FIELDS = {
            "topActivity",
            "topActivityComponent",
            "realActivity",
            "baseActivity",
            "origActivity",
            "activity"
    };

    private static Context systemContext;
    private static Object activityTaskManager;
    private static Method foregroundMethod;
    private static PowerManager powerManager;
    private static Object notificationManager;
    private static Method getZenModeMethod;

    private static String lastStatus = "";

    // -------------------------------------------------------------------------
    // Entry point
    // -------------------------------------------------------------------------

    public static void main(String[] args) {
        setupSystemContext();
        bypassHiddenApiRestrictions();

        if (!initializeServices()) {
            System.err.println("Failed to initialize services, exiting.");
            return;
        }

        runMonitorLoop();
    }

    private static void runMonitorLoop() {
        while (true) {
            try {
                writeStatus();
                Thread.sleep(POLL_INTERVAL_MS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            } catch (Throwable t) {
                t.printStackTrace();
            }
        }
    }

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    private static void setupSystemContext() {
        try {
            Looper.prepare();
            Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
            Object thread = activityThreadClass.getMethod("systemMain").invoke(null);
            systemContext = (Context) activityThreadClass.getMethod("getSystemContext").invoke(thread);
        } catch (Exception e) {
            System.err.println("Failed to set up system context: " + e.getMessage());
        }
    }

    private static void bypassHiddenApiRestrictions() {
        try {
            Class<?> vmRuntime = Class.forName("dalvik.system.VMRuntime");
            Object runtime = vmRuntime.getDeclaredMethod("getRuntime").invoke(null);
            Method setExemptions = vmRuntime.getDeclaredMethod("setHiddenApiExemptions", String[].class);
            setExemptions.invoke(runtime, new Object[]{new String[]{"L"}});
        } catch (Exception e) {
            System.err.println("Failed to bypass hidden API restrictions: " + e.getMessage());
        }
    }

    private static boolean initializeServices() {
        try {
            powerManager = (PowerManager) systemContext.getSystemService(Context.POWER_SERVICE);
            initActivityTaskManager();
            initNotificationManager();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    private static void initActivityTaskManager() throws Exception {
        IBinder binder = getSystemService(resolveAtmServiceName());
        activityTaskManager = bindInterface(resolveAtmInterfaceName() + "$Stub", binder);
        foregroundMethod = findForegroundMethod(activityTaskManager);
    }

    private static void initNotificationManager() throws Exception {
        IBinder binder = getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager = bindInterface("android.app.INotificationManager$Stub", binder);
        getZenModeMethod = notificationManager.getClass().getMethod("getZenMode");
    }

    private static String resolveAtmServiceName() {
        return Build.VERSION.SDK_INT >= 29 ? "activity_task" : Context.ACTIVITY_SERVICE;
    }

    private static String resolveAtmInterfaceName() {
        return Build.VERSION.SDK_INT >= 29
                ? "android.app.IActivityTaskManager"
                : "android.app.IActivityManager";
    }

    private static IBinder getSystemService(String name) throws Exception {
        Class<?> serviceManager = Class.forName("android.os.ServiceManager");
        return (IBinder) serviceManager.getMethod("getService", String.class).invoke(null, name);
    }

    private static Object bindInterface(String stubClassName, IBinder binder) throws Exception {
        return Class.forName(stubClassName)
                .getMethod("asInterface", IBinder.class)
                .invoke(null, binder);
    }

    private static Method findForegroundMethod(Object atm) {
        List<String> candidates = Arrays.asList(FOREGROUND_METHOD_CANDIDATES);
        for (Method method : atm.getClass().getDeclaredMethods()) {
            if (candidates.contains(method.getName())) {
                method.setAccessible(true);
                return method;
            }
        }
        return null;
    }

    // -------------------------------------------------------------------------
    // Status writing
    // -------------------------------------------------------------------------

    private static void writeStatus() {
        String currentStatus = buildStatus();
        if (currentStatus.equals(lastStatus)) {
            return;
        }

        try {
            writeToFile(OUTPUT_PATH, currentStatus);
            lastStatus = currentStatus;
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static String buildStatus() {
        String focusedApp = getFocusedAppInfo();
        int screenAwake = isScreenAwake() ? 1 : 0;
        int batterySaver = isBatterySaverOn() ? 1 : 0;
        int zenMode = getZenMode();

        return "focused_app " + focusedApp + "\n"
                + "screen_awake " + screenAwake + "\n"
                + "battery_saver " + batterySaver + "\n"
                + "zen_mode " + zenMode + "\n";
    }

    private static boolean isScreenAwake() {
        return powerManager != null && powerManager.isInteractive();
    }

    private static boolean isBatterySaverOn() {
        return powerManager != null && powerManager.isPowerSaveMode();
    }

    private static int getZenMode() {
        try {
            if (notificationManager != null && getZenModeMethod != null) {
                return (int) getZenModeMethod.invoke(notificationManager);
            }
        } catch (Exception ignored) {
        }
        return 0;
    }

    private static void writeToFile(String path, String content) throws IOException {
        File file = new File(path);
        File parent = file.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(file))) {
            writer.write(content);
        }
    }

    // -------------------------------------------------------------------------
    // Foreground app detection
    // -------------------------------------------------------------------------

    private static String getFocusedAppInfo() {
        try {
            Object result = invokeForegroundMethod();
            if (result == null) {
                return UNKNOWN_APP;
            }
            if (result instanceof List) {
                return getFocusedAppFromList((List<?>) result);
            }
            return resolveAppInfoFromObject(result);
        } catch (Exception e) {
            e.printStackTrace();
            return UNKNOWN_APP;
        }
    }

    private static String getFocusedAppFromList(List<?> list) {
        if (list.isEmpty()) {
            return NONE_APP;
        }
        for (Object element : list) {
            ComponentName component = extractComponentName(element);
            if (component != null) {
                return buildAppInfo(component.getPackageName());
            }
        }
        return resolveAppInfoFromObject(list.get(0));
    }

    private static String resolveAppInfoFromObject(Object obj) {
        ComponentName component = extractComponentName(obj);
        if (component != null) {
            return buildAppInfo(component.getPackageName());
        }
        String pkg = findPackageLikeString(obj);
        return pkg != null ? buildAppInfo(pkg) : UNKNOWN_APP;
    }

    private static Object invokeForegroundMethod() {
        if (foregroundMethod == null) {
            return null;
        }

        Object result = tryInvokeForegroundMethod();
        if (result != null) {
            return result;
        }

        return bruteForceForegroundMethod();
    }

    private static Object tryInvokeForegroundMethod() {
        String name = foregroundMethod.getName();
        try {
            if (name.equals("getTasks") || name.equals("getRunningTasks")) {
                return tryInvokeWithArgs(foregroundMethod, activityTaskManager,
                        new Object[]{1},
                        new Object[]{1, 0},
                        new Object[]{1, false, false});
            } else if (foregroundMethod.getParameterTypes().length == 0) {
                return foregroundMethod.invoke(activityTaskManager);
            } else {
                return tryInvokeWithArgs(foregroundMethod, activityTaskManager, new Object[]{0});
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    @SafeVarargs
    private static Object tryInvokeWithArgs(Method method, Object target, Object[]... argSets) {
        for (Object[] args : argSets) {
            try {
                return method.invoke(target, args);
            } catch (IllegalArgumentException ignored) {
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    private static Object bruteForceForegroundMethod() {
        try {
            for (Method method : activityTaskManager.getClass().getDeclaredMethods()) {
                String nameLower = method.getName().toLowerCase();
                if (!nameLower.contains("focus") && !nameLower.contains("top") && !nameLower.contains("task")) {
                    continue;
                }
                method.setAccessible(true);
                Object result = tryInvokeNoArgOrSingleInt(method, activityTaskManager);
                if (result != null) {
                    return result;
                }
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    private static Object tryInvokeNoArgOrSingleInt(Method method, Object target) {
        try {
            if (method.getParameterTypes().length == 0) {
                return method.invoke(target);
            } else if (method.getParameterTypes().length == 1) {
                return method.invoke(target, 1);
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    // -------------------------------------------------------------------------
    // Reflection helpers
    // -------------------------------------------------------------------------

    private static ComponentName extractComponentName(Object obj) {
        if (obj == null) return null;
        if (obj instanceof ComponentName) return (ComponentName) obj;

        // Check known field names first
        for (String fieldName : COMPONENT_NAME_FIELDS) {
            ComponentName result = getComponentNameFromField(obj, obj.getClass(), fieldName);
            if (result != null) return result;
        }

        // Fall back to scanning all declared fields in the class hierarchy
        return scanHierarchyForComponentName(obj);
    }

    private static ComponentName getComponentNameFromField(Object obj, Class<?> cls, String fieldName) {
        try {
            Field field = cls.getDeclaredField(fieldName);
            field.setAccessible(true);
            Object value = field.get(obj);
            if (value instanceof ComponentName) return (ComponentName) value;
        } catch (Exception ignored) {
        }
        return null;
    }

    private static ComponentName scanHierarchyForComponentName(Object obj) {
        Class<?> cls = obj.getClass();
        while (cls != null && cls != Object.class) {
            for (Field field : cls.getDeclaredFields()) {
                try {
                    field.setAccessible(true);
                    Object value = field.get(obj);
                    if (value instanceof ComponentName) return (ComponentName) value;
                } catch (Exception ignored) {
                }
            }
            cls = cls.getSuperclass();
        }
        return null;
    }

    private static String findPackageLikeString(Object obj) {
        if (obj == null) return null;

        try {
            String pkg = extractPackageName(obj.toString());
            if (pkg != null) return pkg;
        } catch (Exception ignored) {
        }

        try {
            for (Field field : obj.getClass().getDeclaredFields()) {
                if (!field.getType().equals(String.class)) continue;
                try {
                    field.setAccessible(true);
                    Object value = field.get(obj);
                    if (value instanceof String) {
                        String pkg = extractPackageName((String) value);
                        if (pkg != null) return pkg;
                    }
                } catch (Exception ignored) {
                }
            }
        } catch (Exception ignored) {
        }

        return null;
    }

    /**
     * Extracts the first token that looks like a Java package name (e.g. "com.example.app").
     */
    private static String extractPackageName(String input) {
        if (input == null || input.indexOf('.') <= 0) return null;

        String normalized = input.replaceAll("[^a-z0-9._-]", " ");
        for (String token : normalized.split("\\s+")) {
            if (token.contains(".") && token.matches("[a-z0-9]+(\\.[a-z0-9]+)+")) {
                return token;
            }
        }
        return null;
    }

    // -------------------------------------------------------------------------
    // Process info
    // -------------------------------------------------------------------------

    private static String buildAppInfo(String pkg) {
        String pidUid = getPidUid(pkg);
        return pkg + " " + pidUid;
    }

    private static String getPidUid(String pkg) {
        try {
            ActivityManager am = (ActivityManager) systemContext.getSystemService(Context.ACTIVITY_SERVICE);
            List<ActivityManager.RunningAppProcessInfo> processes = am.getRunningAppProcesses();
            if (processes != null) {
                for (ActivityManager.RunningAppProcessInfo process : processes) {
                    if (pkg.equals(process.processName) || containsPackage(process.pkgList, pkg)) {
                        return process.pid + " " + process.uid;
                    }
                }
            }
        } catch (Exception ignored) {
        }
        return "0 0";
    }

    private static boolean containsPackage(String[] pkgList, String pkg) {
        if (pkgList == null) return false;
        for (String entry : pkgList) {
            if (pkg.equals(entry)) return true;
        }
        return false;
    }
}
