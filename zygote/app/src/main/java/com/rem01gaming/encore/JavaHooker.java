package com.rem01gaming.encore.hook;

import android.os.Build;
import android.util.Log;
import android.util.Pair;

import com.v7878.unsafe.ArtMethodUtils;
import com.v7878.unsafe.Reflection;
import com.v7878.unsafe.invoke.EmulatedStackFrame;
import com.v7878.unsafe.invoke.Transformers;
import com.v7878.vmtools.HookTransformer;
import com.v7878.vmtools.Hooks;

import java.io.BufferedReader;
import java.io.FileReader;
import java.lang.invoke.MethodHandle;
import java.lang.reflect.Executable;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Improved single-class reusable Java hooker.
 * <p>
 * Example:
 * JavaHooker.hook("com.example.Target")
 * .method("isPremium")
 * .before(ctx -> ctx.setResult(true))
 * .install();
 */
public final class JavaHooker {

    private static final int TIRAMISU = 33;
    private static final int RETURN_VALUE_IDX = resolveReturnValueIndex();
    private static final List<HookHandle> ALIVE = new CopyOnWriteArrayList<>();

    private static volatile ClassLoader defaultClassLoader = ClassLoader.getSystemClassLoader();
    private static volatile HookLogger logger = HookLogger.NONE;

    /**
     * If true, exceptions thrown by hooks/original methods will have their stack traces cleared.
     * Useful for stealth, bad for debugging.
     */
    public static volatile boolean clearStackTraces = false;

    private JavaHooker() {
    }

    // ---------------------------------------------------------------------
    // Public configuration
    // ---------------------------------------------------------------------

    public static void setDefaultClassLoader(ClassLoader loader) {
        defaultClassLoader = loader != null ? loader : ClassLoader.getSystemClassLoader();
    }

    public static ClassLoader getDefaultClassLoader() {
        return defaultClassLoader;
    }

    public static void setLogger(HookLogger hookLogger) {
        logger = hookLogger != null ? hookLogger : HookLogger.NONE;
    }

    public static HookLogger getLogger() {
        return logger;
    }

    // ---------------------------------------------------------------------
    // Logger
    // ---------------------------------------------------------------------

    public interface HookLogger {
        void debug(String message);

        void warn(String message);

        void error(String message, Throwable t);

        HookLogger NONE = new HookLogger() {
            @Override
            public void debug(String message) {
            }

            @Override
            public void warn(String message) {
            }

            @Override
            public void error(String message, Throwable t) {
            }
        };
    }

    public static final class LogcatLogger implements HookLogger {
        private final String tag;

        public LogcatLogger(String tag) {
            this.tag = tag != null ? tag : "JavaHooker";
        }

        @Override
        public void debug(String message) {
            Log.d(tag, message);
        }

        @Override
        public void warn(String message) {
            Log.w(tag, message);
        }

        @Override
        public void error(String message, Throwable t) {
            Log.e(tag, message, t);
        }
    }

    // ---------------------------------------------------------------------
    // Process helpers
    // ---------------------------------------------------------------------

    public static final class ProcessInfo {

        private ProcessInfo() {
        }

        public static String currentBasePackage() {
            String process = readProcessName();
            if (process == null || process.isEmpty()) {
                return "unknown.process";
            }

            int colon = process.indexOf(':');
            if (colon > 0) {
                process = process.substring(0, colon);
            }

            return process;
        }

        public static boolean isAppProcess() {
            String p = currentBasePackage();
            return p.contains(".") && !p.equals("system_server") && !p.equals("zygote") && !p.equals("zygote64");
        }

        private static String readProcessName() {
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader("/proc/self/cmdline"));
                StringBuilder sb = new StringBuilder();

                int c;
                while ((c = reader.read()) != -1 && c != 0) {
                    sb.append((char) c);
                }

                return sb.toString().trim();
            } catch (Throwable t) {
                return null;
            } finally {
                if (reader != null) {
                    try {
                        reader.close();
                    } catch (Throwable ignored) {
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // Scope
    // ---------------------------------------------------------------------

    public interface HookScope {
        boolean shouldInstall();
    }

    public static HookScope anyScope() {
        return new HookScope() {
            @Override
            public boolean shouldInstall() {
                return true;
            }
        };
    }

    public static HookScope packageIs(final String packageName) {
        return new HookScope() {
            @Override
            public boolean shouldInstall() {
                return packageName != null && packageName.equals(ProcessInfo.currentBasePackage());
            }
        };
    }

    public static HookScope packages(final String... packageNames) {
        return new HookScope() {
            @Override
            public boolean shouldInstall() {
                if (packageNames == null || packageNames.length == 0) {
                    return false;
                }

                String current = ProcessInfo.currentBasePackage();
                for (String pkg : packageNames) {
                    if (pkg != null && pkg.equals(current)) {
                        return true;
                    }
                }

                return false;
            }
        };
    }

    // ---------------------------------------------------------------------
    // Hook callbacks
    // ---------------------------------------------------------------------

    public interface BeforeHook {
        void before(HookContext ctx) throws Throwable;
    }

    public interface AfterHook {
        void after(HookContext ctx) throws Throwable;
    }

    // ---------------------------------------------------------------------
    // Entry points
    // ---------------------------------------------------------------------

    public static Builder hook(String className) {
        return new Builder(className);
    }

    public static Builder hook(Class<?> clazz) {
        return new Builder(clazz.getName()).classLoader(clazz.getClassLoader());
    }

    public static HookHandle hookBefore(String className, String methodName, BeforeHook hook) {
        return hook(className).method(methodName).before(hook).install();
    }

    public static HookHandle hookAfter(String className, String methodName, AfterHook hook) {
        return hook(className).method(methodName).after(hook).install();
    }

    // ---------------------------------------------------------------------
    // Builder
    // ---------------------------------------------------------------------

    public static final class Builder {
        private final String className;
        private String methodName;
        private int paramCount = -1;
        private Class<?>[] parameterTypes;
        private ClassLoader loader;
        private BeforeHook before;
        private AfterHook after;
        private HookScope scope;
        private boolean optional = false;

        private Builder(String className) {
            if (className == null || className.isEmpty()) {
                throw new IllegalArgumentException("className is empty");
            }
            this.className = className;
        }

        public Builder method(String methodName) {
            this.methodName = methodName;
            return this;
        }

        public Builder paramCount(int paramCount) {
            this.paramCount = paramCount;
            return this;
        }

        public Builder parameters(Class<?>... parameterTypes) {
            this.parameterTypes = parameterTypes == null ? null : parameterTypes.clone();
            return this;
        }

        public Builder classLoader(ClassLoader loader) {
            this.loader = loader;
            return this;
        }

        public Builder before(BeforeHook before) {
            this.before = before;
            return this;
        }

        public Builder after(AfterHook after) {
            this.after = after;
            return this;
        }

        public Builder scope(HookScope scope) {
            this.scope = scope;
            return this;
        }

        /**
         * If true, installation failures return a non-installed handle instead of throwing.
         */
        public Builder optional(boolean optional) {
            this.optional = optional;
            return this;
        }

        public HookHandle install() {
            return JavaHooker.install(this);
        }
    }

    // ---------------------------------------------------------------------
    // Hook handle
    // ---------------------------------------------------------------------

    public static final class HookHandle {
        public final String className;
        public final String methodName;
        public final int paramCount;
        public final Class<?>[] parameterTypes;
        public final ClassLoader loader;
        public final boolean optional;

        final HookScope scope;
        final BeforeHook before;
        final AfterHook after;

        volatile boolean enabled = true;
        volatile Method method;
        volatile Pair<Long, Long> memoryAddresses;
        volatile boolean installed;
        volatile boolean scopeBlocked;
        volatile String lastError;

        HookHandle(Builder b) {
            this.className = b.className;
            this.methodName = b.methodName;
            this.paramCount = b.paramCount;
            this.parameterTypes = b.parameterTypes == null ? null : b.parameterTypes.clone();
            this.loader = b.loader;
            this.before = b.before;
            this.after = b.after;
            this.scope = b.scope;
            this.optional = b.optional;
        }

        public void setEnabled(boolean enabled) {
            this.enabled = enabled;
        }

        public boolean isEnabled() {
            return enabled;
        }

        public boolean isInstalled() {
            return installed;
        }

        public boolean isScopeBlocked() {
            return scopeBlocked;
        }

        public String lastError() {
            return lastError;
        }

        @Override
        public String toString() {
            return className + "#" + methodName;
        }
    }

    // ---------------------------------------------------------------------
    // Hook context
    // ---------------------------------------------------------------------

    public static final class HookContext {
        private final HookHandle handle;
        private final EmulatedStackFrame frame;
        private final ReturnValue value;

        HookContext(HookHandle handle, EmulatedStackFrame frame, ReturnValue value) {
            this.handle = handle;
            this.frame = frame;
            this.value = value;
        }

        public String className() {
            return handle.className;
        }

        public String methodName() {
            return handle.methodName;
        }

        public Method method() {
            return handle.method;
        }

        public boolean isEnabled() {
            return handle.enabled;
        }

        /**
         * Number of slots in the emulated frame.
         * For instance methods this usually includes "this" at index 0.
         */
        public int argCount() {
            return frame.type().parameterCount();
        }

        public boolean hasArg(int index) {
            return index >= 0 && index < argCount();
        }

        public Class<?> returnType() {
            return frame.type().returnType();
        }

        public int declaredParameterCount() {
            return handle.method != null ? handle.method.getParameterCount() : -1;
        }

        /**
         * Usually index 0 for instance methods.
         * Be careful with static methods.
         */
        public Object thisObject() {
            return argCount() > 0 ? arg(0) : null;
        }

        public Object arg(int index) {
            return getArgument(frame, index);
        }

        public void setArg(int index, Object value) {
            setArgument(frame, index, value);
        }

        public String stringArg(int index) {
            if (!hasArg(index)) return null;
            Object value = arg(index);
            return value instanceof String ? (String) value : null;
        }

        public byte[] byteArrayArg(int index) {
            if (!hasArg(index)) return null;
            Object value = arg(index);
            return value instanceof byte[] ? (byte[]) value : null;
        }

        public boolean booleanArg(int index) {
            if (!hasArg(index)) return false;
            Object value = arg(index);
            return value instanceof Boolean && (Boolean) value;
        }

        public int intArg(int index) {
            if (!hasArg(index)) return 0;
            Object value = arg(index);
            return value instanceof Number ? ((Number) value).intValue() : 0;
        }

        public long longArg(int index) {
            if (!hasArg(index)) return 0L;
            Object value = arg(index);
            return value instanceof Number ? ((Number) value).longValue() : 0L;
        }

        public Object result() {
            return value.result;
        }

        /**
         * Replaces the original return value.
         * If called in a before-hook, the original method is skipped.
         */
        public void setResult(Object result) {
            value.setUserResult(result);
        }

        /**
         * Safe helper for DRM-style byte[] spoofing.
         * Always clones the array before returning it.
         */
        public void setByteArrayResultClone(byte[] result) {
            if (result == null) {
                setResult(null);
            } else {
                setResult(result.clone());
            }
        }

        public void setStringResult(String result) {
            setResult(result);
        }

        public boolean isReplaced() {
            return value.userReplaced;
        }

        public Throwable error() {
            return value.error;
        }

        /**
         * Set to null to swallow an exception from the original method.
         */
        public void setError(Throwable error) {
            value.error = error;
        }

        public boolean isOriginalError() {
            return value.originalError;
        }

        public boolean isHookError() {
            return value.hookError;
        }
    }

    // ---------------------------------------------------------------------
    // Internals
    // ---------------------------------------------------------------------

    private static final class ReturnValue {
        Object result;
        boolean hasResult;
        boolean userReplaced;
        Throwable error;
        boolean originalError;
        boolean hookError;

        void setUserResult(Object value) {
            this.result = value;
            this.hasResult = true;
            this.userReplaced = true;
        }

        void captureOriginalResult(Object value) {
            this.result = value;
            this.hasResult = true;
        }
    }

    private interface TransformerCallback {
        void call(MethodHandle original, EmulatedStackFrame frame) throws Throwable;
    }

    private static HookHandle install(Builder b) {
        if (b.methodName == null || b.methodName.isEmpty()) {
            throw new IllegalArgumentException("methodName is empty");
        }

        if (b.before == null && b.after == null) {
            throw new IllegalArgumentException("At least one of before/after hook is required");
        }

        final HookHandle handle = new HookHandle(b);

        if (handle.scope != null && !handle.scope.shouldInstall()) {
            handle.scopeBlocked = true;
            logger.debug("Scope blocked install: " + handle);
            return handle;
        }

        HookTransformer transformer = createTransformer(new TransformerCallback() {
            @Override
            public void call(MethodHandle original, EmulatedStackFrame frame) throws Throwable {
                dispatch(handle, original, frame);
            }
        });

        boolean ok = applyHook(handle, transformer);
        if (ok) {
            ALIVE.add(handle);
            return handle;
        }

        if (handle.optional) {
            logger.warn("Optional hook not installed: " + handle + ", lastError=" + handle.lastError);
            return handle;
        }

        throw new IllegalStateException("Hook target not installed: " + handle + (handle.paramCount >= 0 ? "(" + handle.paramCount + " params)" : "") + ", lastError=" + handle.lastError);
    }

    private static boolean applyHook(HookHandle handle, HookTransformer transformer) {
        Class<?> current = resolveClass(handle.className, handle.loader);
        if (current == null) {
            handle.lastError = "Class not found: " + handle.className;
            logger.warn(handle.lastError);
            return false;
        }

        while (current != null && current != Object.class) {
            Object hidden;
            try {
                hidden = Reflection.getHiddenExecutables(current);
            } catch (Throwable t) {
                handle.lastError = "Reflection.getHiddenExecutables failed for " + current.getName() + ": " + t;
                logger.debug(handle.lastError);
                current = current.getSuperclass();
                continue;
            }

            List<Executable> candidates = new ArrayList<>();

            for (Executable executable : toExecutableList(hidden)) {
                if (!(executable instanceof Method)) {
                    continue;
                }

                if (Modifier.isAbstract(executable.getModifiers())) {
                    continue;
                }

                if (!handle.methodName.equals(executable.getName())) {
                    continue;
                }

                if (handle.parameterTypes != null) {
                    if (!Arrays.equals(executable.getParameterTypes(), handle.parameterTypes)) {
                        continue;
                    }
                } else if (handle.paramCount >= 0) {
                    if (executable.getParameterCount() != handle.paramCount) {
                        continue;
                    }
                }

                candidates.add(executable);
            }

            if (!candidates.isEmpty()) {
                if (candidates.size() > 1 && handle.parameterTypes == null && handle.paramCount < 0) {
                    handle.lastError = "Ambiguous method: " + handle.className + "#" + handle.methodName + ". Specify paramCount or parameters.";
                    logger.warn(handle.lastError);
                    return false;
                }

                Collections.sort(candidates, new Comparator<Executable>() {
                    @Override
                    public int compare(Executable a, Executable b) {
                        return Integer.compare(a.getParameterCount(), b.getParameterCount());
                    }
                });

                for (Executable executable : candidates) {
                    try {
                        Pair<Long, Long> addresses = Hooks.hook(executable, Hooks.EntryPointType.DIRECT, transformer, Hooks.EntryPointType.DIRECT);

                        handle.method = (Method) executable;
                        handle.memoryAddresses = addresses;
                        handle.installed = true;
                        handle.lastError = null;

                        logger.debug("Hooked: " + executable);
                        return true;
                    } catch (Throwable t) {
                        handle.lastError = "Hooks.hook failed for " + executable + ": " + t;
                        logger.debug(handle.lastError);
                    }
                }
            }

            current = current.getSuperclass();
        }

        if (handle.lastError == null) {
            handle.lastError = "Method not found: " + handle.className + "#" + handle.methodName;
        }

        return false;
    }

    private static Class<?> resolveClass(String className, ClassLoader preferred) {
        ClassLoader[] candidates = new ClassLoader[]{preferred, defaultClassLoader, Thread.currentThread().getContextClassLoader(), ClassLoader.getSystemClassLoader(), null // boot classloader
        };

        for (ClassLoader loader : candidates) {
            try {
                return Class.forName(className, true, loader);
            } catch (ClassNotFoundException ignored) {
            } catch (Throwable t) {
                logger.debug("resolveClass failed for " + className + " with loader " + loader + ": " + t);
            }
        }

        return null;
    }

    private static HookTransformer createTransformer(final TransformerCallback callback) {
        ClassLoader loader = HookTransformer.class.getClassLoader();
        if (loader == null) {
            loader = JavaHooker.class.getClassLoader();
        }
        if (loader == null) {
            loader = ClassLoader.getSystemClassLoader();
        }

        return (HookTransformer) Proxy.newProxyInstance(loader, new Class<?>[]{HookTransformer.class}, new InvocationHandler() {
            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                if (method.getDeclaringClass() == Object.class) {
                    switch (method.getName()) {
                        case "hashCode":
                            return System.identityHashCode(proxy);
                        case "equals":
                            return args != null && args.length > 0 && proxy == args[0];
                        case "toString":
                            return "JavaHookerTransformer";
                        default:
                            return defaultFor(method.getReturnType());
                    }
                }

                if (args != null && args.length == 2 && args[0] instanceof MethodHandle && args[1] instanceof EmulatedStackFrame) {

                    MethodHandle original = (MethodHandle) args[0];
                    EmulatedStackFrame frame = (EmulatedStackFrame) args[1];

                    callback.call(original, frame);

                    Class<?> transformerReturnType = method.getReturnType();
                    if (transformerReturnType == void.class || transformerReturnType == Void.class) {
                        return null;
                    }

                    Class<?> frameReturnType = frame.type().returnType();
                    if (frameReturnType == void.class || frameReturnType == Void.class) {
                        return defaultFor(transformerReturnType);
                    }

                    Object returnValue = frame.accessor().getValue(RETURN_VALUE_IDX);
                    if (returnValue != null) {
                        return returnValue;
                    }

                    return defaultFor(transformerReturnType);
                }

                return defaultFor(method.getReturnType());
            }
        });
    }

    private static void dispatch(HookHandle handle, MethodHandle original, EmulatedStackFrame frame) throws Throwable {
        ReturnValue value = new ReturnValue();
        HookContext ctx = new HookContext(handle, frame, value);

        if (handle.enabled && handle.before != null) {
            try {
                handle.before.before(ctx);
            } catch (Throwable t) {
                value.error = t;
                value.hookError = true;
            }
        }

        if (value.error != null) {
            maybeClearStackTrace(value.error);
            throw value.error;
        }

        if (!value.userReplaced) {
            try {
                invokeOriginal(handle, original, frame, value);
            } catch (Throwable t) {
                value.error = t;
                value.originalError = true;
            }
        }

        if (handle.enabled && handle.after != null) {
            try {
                handle.after.after(ctx);
            } catch (Throwable t) {
                value.error = t;
                value.hookError = true;
            }
        }

        if (value.error != null) {
            maybeClearStackTrace(value.error);
            throw value.error;
        }

        // If original threw but after-hook cleared the error without providing a result,
        // provide a safe default return value.
        if (!value.hasResult) {
            Class<?> returnType = frame.type().returnType();
            if (returnType != void.class && returnType != Void.class) {
                value.captureOriginalResult(defaultFor(returnType));
            }
        }

        boolean shouldWriteReturn = value.userReplaced || value.originalError || Build.VERSION.SDK_INT < TIRAMISU;

        if (value.hasResult && shouldWriteReturn) {
            setReturnValue(frame, value.result);
        }
    }

    private static void invokeOriginal(HookHandle handle, MethodHandle original, EmulatedStackFrame frame, ReturnValue value) throws Throwable {
        if (Build.VERSION.SDK_INT >= TIRAMISU) {
            Transformers.invokeExactNoChecks(original, frame);

            Class<?> returnType = frame.type().returnType();
            if (returnType != void.class && returnType != Void.class) {
                value.captureOriginalResult(frame.accessor().getValue(RETURN_VALUE_IDX));
            } else {
                value.captureOriginalResult(null);
            }
            return;
        }

        Method method = handle.method;
        Pair<Long, Long> addresses = handle.memoryAddresses;

        if (method == null || addresses == null || addresses.first == null || addresses.second == null) {
            throw new IllegalStateException("Hook entrypoint is not available on this Android version");
        }

        ArtMethodUtils.setExecutableEntryPoint(method, addresses.second);
        try {
            boolean isStatic = Modifier.isStatic(method.getModifiers());
            int available = frame.type().parameterCount();
            int required = method.getParameterCount();

            Object receiver = null;
            int start;

            if (isStatic) {
                // If there is an extra receiver-like slot, skip it.
                start = Math.max(0, available - required);
            } else {
                receiver = available > 0 ? getArgument(frame, 0) : null;
                start = 1;
            }

            Class<?>[] types = method.getParameterTypes();
            Object[] args = new Object[types.length];

            int max = Math.max(0, available - start);
            int count = Math.min(types.length, max);

            for (int i = 0; i < count; i++) {
                args[i] = getArgument(frame, start + i);
            }

            for (int i = count; i < types.length; i++) {
                args[i] = defaultFor(types[i]);
            }

            Object result;
            try {
                result = method.invoke(receiver, args);
            } catch (java.lang.reflect.InvocationTargetException e) {
                Throwable cause = e.getCause();
                if (cause != null) {
                    throw cause;
                }
                throw e;
            }

            value.captureOriginalResult(result);
        } finally {
            ArtMethodUtils.setExecutableEntryPoint(method, addresses.first);
        }
    }

    private static List<Executable> toExecutableList(Object hidden) {
        if (hidden == null) {
            return Collections.emptyList();
        }

        if (hidden instanceof Executable[]) {
            return Arrays.asList((Executable[]) hidden);
        }

        if (hidden instanceof List<?>) {
            List<Executable> out = new ArrayList<>();
            for (Object o : (List<?>) hidden) {
                if (o instanceof Executable) {
                    out.add((Executable) o);
                }
            }
            return out;
        }

        return Collections.emptyList();
    }

    private static Object getArgument(EmulatedStackFrame frame, int index) {
        switch (frame.accessor().getArgumentShorty(index)) {
            case 'L':
                return frame.accessor().getReference(index);
            case 'Z':
                return frame.accessor().getBoolean(index);
            case 'B':
                return frame.accessor().getByte(index);
            case 'C':
                return frame.accessor().getChar(index);
            case 'S':
                return frame.accessor().getShort(index);
            case 'I':
                return frame.accessor().getInt(index);
            case 'J':
                return frame.accessor().getLong(index);
            case 'F':
                return frame.accessor().getFloat(index);
            case 'D':
                return frame.accessor().getDouble(index);
            default:
                throw new IllegalStateException("Unknown argument shorty at index " + index);
        }
    }

    private static void setArgument(EmulatedStackFrame frame, int index, Object value) {
        switch (frame.accessor().getArgumentShorty(index)) {
            case 'L':
                frame.accessor().setReference(index, value);
                break;
            case 'Z':
                frame.accessor().setBoolean(index, asBoolean(value));
                break;
            case 'B':
                frame.accessor().setByte(index, asByte(value));
                break;
            case 'C':
                frame.accessor().setChar(index, asChar(value));
                break;
            case 'S':
                frame.accessor().setShort(index, asShort(value));
                break;
            case 'I':
                frame.accessor().setInt(index, asInt(value));
                break;
            case 'J':
                frame.accessor().setLong(index, asLong(value));
                break;
            case 'F':
                frame.accessor().setFloat(index, asFloat(value));
                break;
            case 'D':
                frame.accessor().setDouble(index, asDouble(value));
                break;
            default:
                throw new IllegalStateException("Unknown argument shorty at index " + index);
        }
    }

    private static void setReturnValue(EmulatedStackFrame frame, Object value) {
        Class<?> returnType = frame.type().returnType();
        if (returnType == void.class || returnType == Void.class) {
            return;
        }

        if (value == null && returnType.isPrimitive()) {
            value = defaultFor(returnType);
        }

        frame.accessor().setValue(RETURN_VALUE_IDX, value);
    }

    private static int resolveReturnValueIndex() {
        try {
            Field f = EmulatedStackFrame.class.getField("RETURN_VALUE_IDX");
            f.setAccessible(true);
            return f.getInt(null);
        } catch (Throwable ignored) {
        }

        try {
            Field f = EmulatedStackFrame.class.getDeclaredField("RETURN_VALUE_IDX");
            f.setAccessible(true);
            return f.getInt(null);
        } catch (Throwable ignored) {
        }

        // Common value used by EmulatedStackFrame implementations.
        return -1;
    }

    private static Object defaultFor(Class<?> type) {
        if (type == boolean.class) return Boolean.FALSE;
        if (type == byte.class) return (byte) 0;
        if (type == char.class) return (char) 0;
        if (type == short.class) return (short) 0;
        if (type == int.class) return 0;
        if (type == long.class) return 0L;
        if (type == float.class) return 0.0f;
        if (type == double.class) return 0.0d;
        return null;
    }

    private static boolean asBoolean(Object value) {
        return value instanceof Boolean && (Boolean) value;
    }

    private static byte asByte(Object value) {
        return value instanceof Number ? ((Number) value).byteValue() : (byte) 0;
    }

    private static char asChar(Object value) {
        if (value instanceof Character) {
            return (Character) value;
        }
        if (value instanceof Number) {
            return (char) ((Number) value).intValue();
        }
        return (char) 0;
    }

    private static short asShort(Object value) {
        return value instanceof Number ? ((Number) value).shortValue() : (short) 0;
    }

    private static int asInt(Object value) {
        return value instanceof Number ? ((Number) value).intValue() : 0;
    }

    private static long asLong(Object value) {
        return value instanceof Number ? ((Number) value).longValue() : 0L;
    }

    private static float asFloat(Object value) {
        return value instanceof Number ? ((Number) value).floatValue() : 0.0f;
    }

    private static double asDouble(Object value) {
        return value instanceof Number ? ((Number) value).doubleValue() : 0.0d;
    }

    private static void maybeClearStackTrace(Throwable t) {
        if (clearStackTraces && t != null) {
            t.setStackTrace(new StackTraceElement[0]);
        }
    }
}
