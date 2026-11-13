package com.flexgav.ibu;

import dalvik.system.PathClassLoader;
import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Iterator;
import org.json.JSONArray;
import org.json.JSONObject;

/** Reads the installed manager's model without starting its application or service. */
public final class HmaModel {
    private final Class<?> configClass;

    private HmaModel(String apk, String model) throws Exception {
        ClassLoader loader = new PathClassLoader(apk, HmaModel.class.getClassLoader());
        Class<?> found = null;
        // HMAL releases use two verified namespaces for the same public model.
        for (String name : model.split(":")) {
            try {
                found = Class.forName(name, false, loader);
                break;
            } catch (ClassNotFoundException missing) {
                // Try the other known namespace, never a guessed field mapping.
                continue;
            }
        }
        if (found == null) throw new ClassNotFoundException("No supported HMA config model in APK");
        configClass = found;
    }

    private JSONObject parse(String json) throws Exception {
        Object companion = configClass.getField("Companion").get(null);
        Object config = companion.getClass().getMethod("parse", String.class).invoke(companion, json);
        return new JSONObject(config.toString());
    }

    private JSONObject schema(String baseline) throws Exception {
        JSONObject defaults = new JSONObject(configClass.getConstructor().newInstance().toString());
        JSONObject current = baseline.isEmpty() ? defaults : parse(read(baseline));
        if (current.getInt("configVersion") < 65 || current.getInt("configVersion") > defaults.getInt("configVersion")) {
            throw new IllegalArgumentException("Unsupported existing HMA config version");
        }
        JSONObject result = new JSONObject();
        for (Iterator<String> keys = defaults.keys(); keys.hasNext();) {
            String key = keys.next();
            result.put(key, current.has(key) ? current.get(key) : defaults.get(key));
        }
        // File versions and obsolete keys cannot define the currently installed model.
        result.put("configVersion", defaults.get("configVersion"));
        requireObject(defaults, "templates");
        requireObject(defaults, "scope");
        result.put("templates", new JSONObject());
        if (defaults.has("settingsTemplates")) result.put("settingsTemplates", new JSONObject());
        Class<?> appClass = Class.forName(configClass.getName() + "$AppConfig", true, configClass.getClassLoader());
        JSONObject app = new JSONObject(appClass.getConstructor().newInstance().toString());
        result.put("scope", new JSONObject().put("integritybox.schema", app));
        return result;
    }

    private static void requireObject(JSONObject object, String key) throws Exception {
        if (!(object.opt(key) instanceof JSONObject)) throw new IllegalArgumentException("Missing HMA model block: " + key);
    }

    /** Every supplied setting must survive the native reader and writer. */
    private static void unchanged(Object expected, Object actual, String path) throws Exception {
        if (expected instanceof JSONObject && actual instanceof JSONObject) {
            JSONObject left = (JSONObject) expected;
            JSONObject right = (JSONObject) actual;
            for (Iterator<String> keys = left.keys(); keys.hasNext();) {
                String key = keys.next();
                if (!right.has(key)) throw new IllegalArgumentException("HMA model discarded " + path + "." + key);
                unchanged(left.get(key), right.get(key), path + "." + key);
            }
            return;
        }
        if (expected instanceof JSONArray && actual instanceof JSONArray) {
            JSONArray left = (JSONArray) expected;
            JSONArray right = (JSONArray) actual;
            if (left.length() != right.length()) throw new IllegalArgumentException("HMA model changed " + path);
            for (int i = 0; i < left.length(); i++) unchanged(left.get(i), right.get(i), path + "[" + i + "]");
            return;
        }
        boolean equal = expected instanceof Number && actual instanceof Number
                ? Double.compare(((Number) expected).doubleValue(), ((Number) actual).doubleValue()) == 0
                : expected.equals(actual);
        if (!equal) throw new IllegalArgumentException("HMA model changed " + path);
    }

    private static String read(String path) throws Exception {
        File file = new File(path);
        if (!file.isFile() || file.length() > 1048576) throw new IllegalArgumentException("Invalid HMA JSON file");
        return new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8);
    }

    public static void main(String[] args) {
        try {
            if (args.length != 4) throw new IllegalArgumentException("Expected mode, APK, model and input");
            HmaModel model = new HmaModel(args[1], args[2]);
            if ("schema".equals(args[0])) {
                System.out.println(model.schema(args[3]).toString());
            } else if ("verify".equals(args[0])) {
                String json = read(args[3]);
                unchanged(new JSONObject(json), model.parse(json), "config");
            } else {
                throw new IllegalArgumentException("Unknown HMA model operation");
            }
        } catch (Throwable error) {
            while (error instanceof InvocationTargetException && error.getCause() != null) error = error.getCause();
            // Report the model error, never config contents or application data.
            String detail = error.getClass() == IllegalArgumentException.class ? error.getMessage() : "Cannot read or verify the installed model";
            System.err.println("HMA model: " + error.getClass().getSimpleName() + ": " + detail);
            System.exit(3);
        }
    }
}
