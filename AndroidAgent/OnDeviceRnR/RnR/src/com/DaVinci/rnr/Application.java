package com.DaVinci.rnr;

import android.graphics.drawable.Drawable;

public class Application {
	private Drawable icon;
	private String appName;
	private String packageName;
	private String activityName;
	private boolean isSystemApp;
	private long codesize;
	
	public long getCodesize() {
		return codesize;
	}
	public void setCodesize(long codesize) {
		this.codesize = codesize;
	}
	public Drawable getIcon() {
		return icon;
	}
	public void setIcon(Drawable icon) {
		this.icon = icon;
	}
	public String getAppName() {
		return appName;
	}
	public void setAppName(String appName) {
		this.appName = appName;
	}
	public String getPackageName() {
		return packageName;
	}
	public void setPackageName(String packageName) {
		this.packageName = packageName;
	}
	public String getActivityName() {
		return activityName;
	}
	public void setActivityName(String activityName) {
		this.activityName = activityName;
	}
	public boolean isSystemApp() {
		return isSystemApp;
	}
	public void setSystemApp(boolean isSystemApp) {
		this.isSystemApp = isSystemApp;
	}
}
