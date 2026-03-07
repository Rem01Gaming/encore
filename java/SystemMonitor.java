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
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;

public class SystemMonitor {
    private static final String OUTPUT_PATH = "/data/adb/.config/encore/system_status";
    private static String lastStatus = "";

    private static Context systemContext;
    private static Object activityTaskManager;
    private static Method foregroundMethod;
    private static PowerManager powerManager;
    private static Object notificationManager;
    private static Method getZenModeMethod;

    public static void main(String[] args) {
        setupSystemContext();
        bypassHiddenApiRestrictions();
        if (!initializeServices())
            return;
        while (true) {
            try {
                writeStatus();
                Thread.sleep(500);
            } catch (Throwable t) {
                t.printStackTrace();
            }
        }
    }

    private static void setupSystemContext() {
        try {
            Looper.prepare();
            Class<?> at = Class.forName("android.app.ActivityThread");
            Object thread = at.getMethod("systemMain").invoke(null);
            systemContext = (Context) at.getMethod("getSystemContext").invoke(thread);
        } catch (Exception ignored) {
        }
    }

    private static void bypassHiddenApiRestrictions() {
        try {
            Class<?> vm = Class.forName("dalvik.system.VMRuntime");
            Method getRuntime = vm.getDeclaredMethod("getRuntime");
            Object runtime = getRuntime.invoke(null);
            Method setHiddenApi = vm.getDeclaredMethod("setHiddenApiExemptions", String[].class);
            setHiddenApi.invoke(runtime, new Object[] { new String[] { "L" } });
        } catch (Exception ignored) {
        }
    }

    private static boolean initializeServices() {
        try {
            powerManager = (PowerManager) systemContext.getSystemService(Context.POWER_SERVICE);

            Class<?> sm = Class.forName("android.os.ServiceManager");
            Method getService = sm.getMethod("getService", String.class);

            String atmInterface = Build.VERSION.SDK_INT >= 29 ? "android.app.IActivityTaskManager"
                    : "android.app.IActivityManager";
            String atmService = Build.VERSION.SDK_INT >= 29 ? "activity_task" : Context.ACTIVITY_SERVICE;

            IBinder atmBinder = (IBinder) getService.invoke(null, atmService);
            activityTaskManager = Class.forName(atmInterface + "$Stub").getMethod("asInterface", IBinder.class)
                    .invoke(null, atmBinder);

            String[] candidates = {
                    "getFocusedRootTask",
                    "getFocusedStackInfo",
                    "getFocusedTaskInfo",
                    "getFocusedRootTaskInfo",
                    "getTasks",
                    "getRunningTasks",
                    "getTopActivity"
            };

            for (Method m : activityTaskManager.getClass().getDeclaredMethods()) {
                String name = m.getName();
                for (String c : candidates) {
                    if (name.equals(c)) {
                        m.setAccessible(true);
                        foregroundMethod = m;
                        break;
                    }
                }
                if (foregroundMethod != null)
                    break;
            }

            IBinder nmBinder = (IBinder) getService.invoke(null, Context.NOTIFICATION_SERVICE);
            notificationManager = Class.forName("android.app.INotificationManager$Stub")
                    .getMethod("asInterface", IBinder.class).invoke(null, nmBinder);
            getZenModeMethod = notificationManager.getClass().getMethod("getZenMode");

            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    private static void writeStatus() {
        try {
            String focused = getFocusedAppInfo();
            int awake = (powerManager != null && powerManager.isInteractive()) ? 1 : 0;
            int saver = (powerManager != null && powerManager.isPowerSaveMode()) ? 1 : 0;
            int zen = 0;

            try {
                if (notificationManager != null && getZenModeMethod != null)
                    zen = (int) getZenModeMethod.invoke(notificationManager);
            } catch (Exception ignored) {
            }

            StringBuilder sb = new StringBuilder();
            sb.append("focused_app ").append(focused).append("\n");
            sb.append("screen_awake ").append(awake).append("\n");
            sb.append("battery_saver ").append(saver).append("\n");
            sb.append("zen_mode ").append(zen).append("\n");

            String currentStatus = sb.toString();

            if (!currentStatus.equals(lastStatus)) {
                File file = new File(OUTPUT_PATH);
                if (file.getParentFile() != null && !file.getParentFile().exists())
                    file.getParentFile().mkdirs();

                try (BufferedWriter writer = new BufferedWriter(new FileWriter(file))) {
                    writer.write(currentStatus);
                    writer.flush();
                }

                lastStatus = currentStatus;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static String getFocusedAppInfo() {
        try {
            Object res = invokeForegroundMethod();
            if (res == null)
                return "unknown 0 0";

            // if list, inspect elements
            if (res instanceof List) {
                List<?> list = (List<?>) res;
                if (list.isEmpty())
                    return "none 0 0";

                for (Object elem : list) {
                    ComponentName c = extractComponentName(elem);
                    if (c != null)
                        return pkgPidUid(c.getPackageName());
                }

                // try first element's fields if none found
                ComponentName c0 = extractComponentName(list.get(0));
                if (c0 != null)
                    return pkgPidUid(c0.getPackageName());
                return "unknown 0 0";
            }

            ComponentName c = extractComponentName(res);
            if (c != null)
                return pkgPidUid(c.getPackageName());

            // try to find a package-like string in toString() or string fields
            String pkg = findPackageLikeString(res);
            if (pkg != null)
                return pkgPidUid(pkg);

            return "unknown 0 0";
        } catch (Exception e) {
            e.printStackTrace();
            return "unknown 0 0";
        }
    }

    private static Object invokeForegroundMethod() {
        if (foregroundMethod == null)
            return null;
        try {
            String name = foregroundMethod.getName();
            if (name.equals("getTasks") || name.equals("getRunningTasks")) {
                // try common signatures
                try {
                    return foregroundMethod.invoke(activityTaskManager, 1);
                } catch (IllegalArgumentException ignored) {
                }

                try {
                    return foregroundMethod.invoke(activityTaskManager, 1, 0);
                } catch (IllegalArgumentException ignored) {
                }

                try {
                    return foregroundMethod.invoke(activityTaskManager, 1, false, false);
                } catch (IllegalArgumentException ignored) {
                }
            } else {
                if (foregroundMethod.getParameterTypes().length == 0)
                    return foregroundMethod.invoke(activityTaskManager);

                // try invoking with single int if method wants it
                try {
                    return foregroundMethod.invoke(activityTaskManager, 0);
                } catch (IllegalArgumentException ignored) {
                }
            }
        } catch (Exception e) {
            // fall through to brute-force attempts
        }

        // brute-force: try other ATM methods that look relevant
        try {
            for (Method m : activityTaskManager.getClass().getDeclaredMethods()) {
                String n = m.getName().toLowerCase();
                if (n.contains("focus") || n.contains("top") || n.contains("task")) {
                    try {
                        m.setAccessible(true);
                        if (m.getParameterTypes().length == 0) {
                            Object r = m.invoke(activityTaskManager);
                            if (r != null)
                                return r;
                        } else if (m.getParameterTypes().length == 1) {
                            Object r = m.invoke(activityTaskManager, 1);
                            if (r != null)
                                return r;
                        }
                    } catch (Exception ignored) {
                    }
                }
            }
        } catch (Exception ignored) {
        }

        return null;
    }

    private static ComponentName extractComponentName(Object obj) {
        if (obj == null)
            return null;

        if (obj instanceof ComponentName)
            return (ComponentName) obj;

        String[] common = {
                "topActivity",
                "topActivityComponent",
                "realActivity",
                "baseActivity",
                "origActivity",
                "activity"
        };

        for (String fName : common) {
            try {
                Field f = obj.getClass().getDeclaredField(fName);
                f.setAccessible(true);
                Object v = f.get(obj);
                if (v instanceof ComponentName)
                    return (ComponentName) v;
            } catch (Exception ignored) {
            }
        }

        // scan declared fields
        Class<?> cls = obj.getClass();
        while (cls != null && cls != Object.class) {
            for (Field f : cls.getDeclaredFields()) {
                try {
                    f.setAccessible(true);
                    Object v = f.get(obj);
                    if (v instanceof ComponentName)
                        return (ComponentName) v;
                } catch (Exception ignored) {
                }
            }
            cls = cls.getSuperclass();
        }

        return null;
    }

    private static String findPackageLikeString(Object obj) {
        if (obj == null)
            return null;

        try {
            String s = obj.toString();
            String p = extractPkgFromString(s);
            if (p != null)
                return p;
        } catch (Exception ignored) {
        }

        try {
            for (Field f : obj.getClass().getDeclaredFields()) {
                if (!f.getType().equals(String.class))
                    continue;

                try {
                    f.setAccessible(true);
                    Object v = f.get(obj);
                    if (v instanceof String) {
                        String p = extractPkgFromString((String) v);
                        if (p != null)
                            return p;
                    }
                } catch (Exception ignored) {
                }
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    private static String extractPkgFromString(String s) {
        if (s == null)
            return null;
        // quick heuristic for package names
        int idx = s.indexOf('.');
        if (idx <= 0)
            return null;

        s = s.replaceAll("[^a-z0-9._-]", " ");
        String[] parts = s.split("\\s+");

        for (String part : parts) {
            if (part.contains(".") && part.matches("[a-z0-9]+(\\.[a-z0-9]+)+"))
                return part;
        }

        return null;
    }

    private static String pkgPidUid(String pkg) {
        String piduid = getPidUid(pkg);
        return pkg + " " + (piduid != null ? piduid : "0 0");
    }

    private static String getPidUid(String pkg) {
        try {
            ActivityManager am = (ActivityManager) systemContext.getSystemService(Context.ACTIVITY_SERVICE);
            List<ActivityManager.RunningAppProcessInfo> procs = am.getRunningAppProcesses();
            if (procs != null) {
                for (ActivityManager.RunningAppProcessInfo p : procs) {
                    if (pkg.equals(p.processName) || contains(p.pkgList, pkg)) {
                        return p.pid + " " + p.uid;
                    }
                }
            }
        } catch (Exception ignored) {
        }

        return "0 0";
    }

    private static boolean contains(String[] list, String pkg) {
        if (list == null)
            return false;

        for (String s : list)
            if (pkg.equals(s))
                return true;

        return false;
    }
}
