// First-party source (IntegrityBox, PlayIntegrityFork lineage).
// GMS/DroidGuard attestation entry point. Native Build writes precede
// initAttestation(...); init/receiveJson remain the legacy native/dex ABI.
// Keep public entry descriptors stable (see sources/dex/README.md).
//
// Divergences from upstream:
//  - No org.lsposed.hiddenapibypass dependency; the hidden-API lift is inlined
//    in liftHiddenApiRestrictions() (meta-reflection -> VMRuntime).
//  - Logging via the shared Logger (S1); Build-field writes via BuildFields
//    (S2 + P2 field cache); spoofDevice() applies once per profile (P1 guard).
package es.chiteroman.playintegrityfix;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.Base64;
import android.util.JsonReader;
import java.io.StringReader;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.security.KeyStore;
import java.security.KeyStoreSpi;
import java.security.Provider;
import java.security.Security;
import java.util.HashMap;
import java.util.Map;

public final class EntryPoint {
    private static final Logger log = new Logger("PIF/Java:DG");
    private static int spoofBuildEnabled = 1;
    // P1: spoofDevice() applies the whole Build profile once per received JSON.
    // CustomProvider.getService() may call it on every KeyStore lookup (hot
    // path) — this guard turns those repeats into a no-op. receiveJson() resets
    // it so a re-injected profile is applied again.
    private static volatile boolean deviceSpoofed = false;

    private static final String signatureData = "MIIFyTCCA7GgAwIBAgIVALyxxl+zDS9SL68SzOr48309eAZyMA0GCSqGSIb3DQEBCwUAMHQxCzAJ\n" +
            "BgNVBAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRYwFAYDVQQHEw1Nb3VudGFpbiBWaWV3MRQw\n" +
            "EgYDVQQKEwtHb29nbGUgSW5jLjEQMA4GA1UECxMHQW5kcm9pZDEQMA4GA1UEAxMHQW5kcm9pZDAg\n" +
            "Fw0yMjExMDExODExMzVaGA8yMDUyMTEwMTE4MTEzNVowdDELMAkGA1UEBhMCVVMxEzARBgNVBAgT\n" +
            "CkNhbGlmb3JuaWExFjAUBgNVBAcTDU1vdW50YWluIFZpZXcxFDASBgNVBAoTC0dvb2dsZSBJbmMu\n" +
            "MRAwDgYDVQQLEwdBbmRyb2lkMRAwDgYDVQQDEwdBbmRyb2lkMIICIjANBgkqhkiG9w0BAQEFAAOC\n" +
            "Ag8AMIICCgKCAgEAsqtalIy/nctKlrhd1UVoDffFGnDf9GLi0QQhsVoJkfF16vDDydZJOycG7/kQ\n" +
            "ziRZhFdcoMrIYZzzw0ppBjsSe1AiWMuKXwTBaEtxN99S1xsJiW4/QMI6N6kMunydWRMsbJ6aAxi1\n" +
            "lVq0bxSwr8Sg/8u9HGVivfdG8OpUM+qjuV5gey5xttNLK3BZDrAlco8RkJZryAD40flmJZrWXJmc\n" +
            "r2HhJJUnqG4Z3MSziEgW1u1JnnY3f/BFdgYsA54SgdUGdQP3aqzSjIpGK01/vjrXvifHazSANjvl\n" +
            "0AUE5i6AarMw2biEKB2ySUDp8idC5w12GpqDrhZ/QkW8yBSa87KbkMYXuRA2Gq1fYbQx3YJraw0U\n" +
            "gZ4M3fFKpt6raxxM5j0sWHlULD7dAZMERvNESVrKG3tQ7B39WAD8QLGYc45DFEGOhKv5Fv8510h5\n" +
            "sXK502IvGpI4FDwz2rbtAgJ0j+16db5wCSW5ThvNPhCheyciajc8dU1B5tJzZN/ksBpzne4Xf9gO\n" +
            "LZ9ZU0+3Z5gHVvTS/YpxBFwiFpmL7dvGxew0cXGSsG5UTBlgr7i0SX0WhY4Djjo8IfPwrvvA0QaC\n" +
            "FamdYXKqBsSHgEyXS9zgGIFPt2jWdhaS+sAa//5SXcWro0OdiKPuwEzLgj759ke1sHRnvO735dYn\n" +
            "5whVbzlGyLBh3L0CAwEAAaNQME4wDAYDVR0TBAUwAwEB/zAdBgNVHQ4EFgQUU1eXQ7NoYKjvOQlh\n" +
            "5V8jHQMoxA8wHwYDVR0jBBgwFoAUU1eXQ7NoYKjvOQlh5V8jHQMoxA8wDQYJKoZIhvcNAQELBQAD\n" +
            "ggIBAHFIazRLs3itnZKllPnboSd6sHbzeJURKehx8GJPvIC+xWlwWyFO5+GHmgc3yh/SVd3Xja/k\n" +
            "8Ud59WEYTjyJJWTw0Jygx37rHW7VGn2HDuy/x0D+els+S8HeLD1toPFMepjIXJn7nHLhtmzTPlDW\n" +
            "DrhiaYsls/k5Izf89xYnI4euuOY2+1gsweJqFGfbznqyqy8xLyzoZ6bvBJtgeY+G3i/9Be14HseS\n" +
            "Na4FvI1Oze/l2gUu1IXzN6DGWR/lxEyt+TncJfBGKbjafYrfSh3zsE4N3TU7BeOL5INirOMjre/j\n" +
            "VgB1YQG5qLVaPoz6mdn75AbBBm5a5ahApLiKqzy/hP+1rWgw8Ikb7vbUqov/bnY3IlIU6XcPJTCD\n" +
            "b9aRZQkStvYpQd82XTyxD/T0GgRLnUj5Uv6iZlikFx1KNj0YNS2T3gyvL++J9B0Y6gAkiG0EtNpl\n" +
            "z7Pomsv5pVdmHVdKMjqWw5/6zYzVmu5cXFtR384Ti1qwML1xkD6TC3VIv88rKIEjrkY2c+v1frh9\n" +
            "fRJ2OmzXmML9NgHTjEiJR2Ib2iNrMKxkuTIs9oxKZgrJtJKvdU9qJJKM5PnZuNuHhGs6A/9gt9Oc\n" +
            "cetYeQvVSqeEmQluWfcunQn9C9Vwi2BJIiVJh4IdWZf5/e2PlSSQ9CJjz2bKI17pzdxOmjQfE0JS\n" +
            "F7Xt\n";

    private static final Map<String, String> map = new HashMap<>();

    public static int getVerboseLogs() { return log.level; }
    public static int getSpoofBuildEnabled() { return spoofBuildEnabled; }

    public static void init(int logLevel, int spoofBuildVal, int spoofProviderVal, int spoofSignatureVal) {
        log.level = logLevel;
        spoofBuildEnabled = spoofBuildVal;
        log.i("init verbose=" + logLevel + " build=" + spoofBuildVal + " provider=" + spoofProviderVal + " signature=" + spoofSignatureVal);
        if (log.level > 99) logFields();
        if (spoofProviderVal > 0) spoofProvider();
        if (spoofBuildVal > 0) spoofDevice();
        if (spoofSignatureVal > 0) spoofPackageManager();
    }

    // New native path: Build is already applied and the return mask reports
    // actual Java operations, not merely successful invocation of this method.
    public static int initAttestation(int logLevel, int provider, int signature) {
        log.level = logLevel;
        spoofBuildEnabled = 0;
        int applied = 0;
        if (provider > 0 && spoofProvider()) applied |= 1;
        if (signature > 0 && spoofPackageManager()) applied |= 2;
        return applied;
    }

    public static void receiveJson(String data) {
        log.i("receiveJson len=" + (data != null ? data.length() : 0));
        if (data == null || data.isEmpty()) {
            log.e("json empty");
            return;
        }
        map.clear();
        try (JsonReader reader = new JsonReader(new StringReader(data))) {
            reader.beginObject();
            while (reader.hasNext()) {
                String name = reader.nextName();
                String value = reader.nextString();
                map.put(name, value);
                if (log.level > 2) log.d("json " + name + "=" + value);
            }
            reader.endObject();
            log.i("parsed " + map.size() + " fields");
        } catch (Exception e) {
            log.e("json parse failed: " + e.getMessage());
            map.clear();
        }
        deviceSpoofed = false;   // P1: new profile -> allow spoofDevice() to apply again
    }

    private static boolean spoofProvider() {
        log.i("spoof provider");
        try {
            Provider provider = Security.getProvider("AndroidKeyStore");
            if (provider == null) { log.e("provider not found"); return false; }
            KeyStore keyStore = KeyStore.getInstance("AndroidKeyStore");
            Field f = keyStore.getClass().getDeclaredField("keyStoreSpi");
            f.setAccessible(true);
            CustomKeyStoreSpi.keyStoreSpi = (KeyStoreSpi) f.get(keyStore);
            f.setAccessible(false);
            CustomProvider customProvider = new CustomProvider(provider);
            Security.removeProvider("AndroidKeyStore");
            Security.insertProviderAt(customProvider, 1);
            log.i("provider spoofed");
            return Security.getProvider("AndroidKeyStore") == customProvider;
        } catch (Exception e) {
            log.e("provider failed: " + e.getMessage());
            return false;
        }
    }

    static void spoofDevice() {
        if (deviceSpoofed) return;   // P1: already applied for the current profile
        log.i("spoof " + map.size() + " fields");
        for (Map.Entry<String, String> entry : map.entrySet()) {
            BuildFields.set(entry.getKey(), entry.getValue(), log);
        }
        deviceSpoofed = true;
    }

    private static boolean spoofPackageManager() {
        log.i("spoof signatures");
        Signature spoofedSignature = new Signature(Base64.decode(signatureData, Base64.DEFAULT));
        Parcelable.Creator<PackageInfo> customCreator = new CustomPackageInfoCreator(PackageInfo.CREATOR, spoofedSignature);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            try {
                liftHiddenApiRestrictions();
                log.i("hidden api ok");
            } catch (Exception e) {
                log.e("hidden api failed: " + e.getMessage());
            }
        }
        boolean replaced = false;
        try {
            Field creatorField = findField(PackageInfo.class, "CREATOR");
            creatorField.setAccessible(true);
            try {
                creatorField.set(null, customCreator);
            } catch (IllegalAccessException e) {
                // ART may reject final-field reflection and abort on JNI writes.
                // Resolve offsets through Unsafe; never assume an ART layout.
                Class<?> unsafeClass = Class.forName("jdk.internal.misc.Unsafe");
                Field singleton = unsafeClass.getDeclaredField("theUnsafe");
                singleton.setAccessible(true);
                Object unsafe = singleton.get(null);
                Object base = unsafeClass.getMethod("staticFieldBase", Field.class).invoke(unsafe, creatorField);
                long offset = (Long) unsafeClass.getMethod("staticFieldOffset", Field.class).invoke(unsafe, creatorField);
                unsafeClass.getMethod("putObjectVolatile", Object.class, long.class, Object.class)
                        .invoke(unsafe, base, offset, customCreator);
            }
            replaced = creatorField.get(null) == customCreator;
        } catch (Exception e) {
            log.e("creator failed: " + e.getMessage());
        }
        if (replaced) log.i("creator replaced");
        else log.e("creator unchanged");
        clearCaches();
        return replaced;
    }

    // Keep direct reflection as a fallback for runtimes where meta-reflection is blocked.
    private static void liftHiddenApiRestrictions() throws Exception {
        Exception metaFailure;
        try {
            Method getDeclaredMethod = Class.class.getDeclaredMethod(
                    "getDeclaredMethod", String.class, Class[].class);
            Method forName = (Method) getDeclaredMethod.invoke(
                    Class.class, "forName", new Class[]{String.class});
            Class<?> vmRuntimeClass = (Class<?>) forName.invoke(null, "dalvik.system.VMRuntime");
            Method getRuntime = (Method) getDeclaredMethod.invoke(
                    vmRuntimeClass, "getRuntime", null);
            Method setHiddenApiExemptions = (Method) getDeclaredMethod.invoke(
                    vmRuntimeClass, "setHiddenApiExemptions", new Class[]{String[].class});
            Object vmRuntime = getRuntime.invoke(null);
            setHiddenApiExemptions.invoke(vmRuntime, new Object[]{new String[]{"L"}});
            return;
        } catch (Exception e) {
            metaFailure = e;
        }

        try {
            Class<?> vmRuntimeClass = Class.forName("dalvik.system.VMRuntime");
            Method getRuntime = vmRuntimeClass.getDeclaredMethod("getRuntime");
            Method setHiddenApiExemptions = vmRuntimeClass.getDeclaredMethod(
                    "setHiddenApiExemptions", String[].class);
            getRuntime.setAccessible(true);
            setHiddenApiExemptions.setAccessible(true);
            Object vmRuntime = getRuntime.invoke(null);
            setHiddenApiExemptions.invoke(vmRuntime, new Object[]{new String[]{"L"}});
        } catch (Exception fallbackFailure) {
            fallbackFailure.addSuppressed(metaFailure);
            throw fallbackFailure;
        }
    }

    private static void clearCaches() {
        try {
            Field cacheField = findField(PackageManager.class, "sPackageInfoCache");
            cacheField.setAccessible(true);
            Object cache = cacheField.get(null);
            if (cache != null) {
                cache.getClass().getMethod("clear").invoke(cache);
                log.i("cache cleared");
            }
        } catch (Exception e) {
            if (log.level > 1) log.d("cache clear: " + e.getMessage());
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            clearParcelField("mCreators");
            clearParcelField("sPairedCreators");
        }
    }

    private static void clearParcelField(String fieldName) {
        try {
            Field creatorsField = findField(Parcel.class, fieldName);
            creatorsField.setAccessible(true);
            Map<?, ?> creators = (Map<?, ?>) creatorsField.get(null);
            if (creators != null) {
                creators.clear();
                log.i("parcel " + fieldName + " cleared");
            }
        } catch (Exception e) {
            if (log.level > 1) log.d("parcel " + fieldName + ": " + e.getMessage());
        }
    }

    private static Field findField(Class<?> currentClass, String fieldName) throws NoSuchFieldException {
        while (currentClass != null && !currentClass.equals(Object.class)) {
            try { return currentClass.getDeclaredField(fieldName); }
            catch (NoSuchFieldException e) { currentClass = currentClass.getSuperclass(); }
        }
        throw new NoSuchFieldException("Field '" + fieldName + "' not found");
    }

    private static String logParseField(Field field) {
        Object value = null;
        String type = field.getType().getName();
        String name = field.getName();
        try { value = field.get(null); }
        catch (Exception e) { return name + " access: " + e.getMessage(); }
        return type + " " + name + ": " + String.valueOf(value);
    }

    private static void logFields() {
        log.i("=== Build ===");
        for (Field field : Build.class.getDeclaredFields()) {
            field.setAccessible(true);
            log.d("Build " + logParseField(field));
            field.setAccessible(false);
        }
        log.i("=== Build.VERSION ===");
        for (Field field : Build.VERSION.class.getDeclaredFields()) {
            field.setAccessible(true);
            log.d("Build.VERSION " + logParseField(field));
            field.setAccessible(false);
        }
    }

    // Package-private log bridges for the sibling classes (CustomProvider,
    // CustomKeyStoreSpi, CustomPackageInfoCreator) that report through this tag.
    static void LOG(String msg) { log.d(msg); }
    static void LOGE(String msg) { log.e(msg); }
}
