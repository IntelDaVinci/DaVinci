package com.example.qagent;

import android.content.Context;
import java.lang.reflect.Method;

public class SystemProperty {
    private final Context mContext;

    public SystemProperty(Context mContext) {
        this.mContext = mContext;
    }

    public String get(String key) {
        String result = "";
        try {
            ClassLoader classLoader = mContext.getClassLoader();
            Class<?> SystemProperties = classLoader.loadClass("android.os.SystemProperties");
            Method methodGet = SystemProperties.getMethod("get", String.class);
            result = (String) methodGet.invoke(SystemProperties, key);
        } catch (Exception e) {
        }
        return result;
    }
}