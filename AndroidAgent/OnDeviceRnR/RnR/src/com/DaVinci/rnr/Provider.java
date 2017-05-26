package com.DaVinci.rnr;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;

public class Provider {
	private PackageManager packageManager;
	public Provider(Context context) {
		packageManager = context.getPackageManager();
	}
	/*
	 * get all applications information for the launcher,
	 * although an application is build-in or not has no usage currently,
	 * still keep it for future. 
	 */
	public List<Application> getAllApps() {
		
		List<Application> list = new ArrayList<Application>();
		Application application;
		
		List<PackageInfo> packageInfos = packageManager.getInstalledPackages(0);
		for (PackageInfo info:packageInfos) {
			Intent intent = packageManager.getLaunchIntentForPackage(info.packageName);
			if (intent != null) {
				application = new Application();
				String packageName = info.packageName;
				ApplicationInfo appInfo = info.applicationInfo;
				Drawable icon = appInfo.loadIcon(packageManager);
				String appName = appInfo.loadLabel(packageManager).toString();
				String activityName = intent.getComponent().getClassName().toString();
				application.setPackageName(packageName);
				application.setAppName(appName);
				application.setActivityName(activityName);
				application.setIcon(icon);
				
				if (isBuildin(appInfo)) {
					application.setSystemApp(false);
				} else {
					application.setSystemApp(true);
				}
				list.add(application);
			}
		}
		return list;
	}

	public boolean isBuildin(ApplicationInfo info) {
		if ((info.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0) {
			/*
			 * application has been install as an update to a built-in system application.
			 */
			return true;
		}else if((info.flags & ApplicationInfo.FLAG_SYSTEM) != 0){
			/*
			 * application is installed in the device's system image.
			 */
			return true;
		}
		return false;
	}
}
