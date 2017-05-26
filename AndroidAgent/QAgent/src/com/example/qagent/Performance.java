package com.example.qagent;

import android.app.ActivityManager;
import android.app.ActivityManager.MemoryInfo;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.Environment;
import android.os.StatFs;

import java.io.RandomAccessFile;

public class Performance {
    private final Context mContext;

    public Performance(Context mContext) {
        this.mContext = mContext;
    }

    public String getCpuUsage()  {
        long cpuUsage = 0;
        
        try {
            RandomAccessFile reader = new RandomAccessFile("/proc/stat", "r");
            String load = reader.readLine();

            String[] toks_start = load.split(" +");  // Split on one or more spaces

            if (toks_start.length > 9)
            {
                long workjiffies_start = Long.parseLong(toks_start[2]) + Long.parseLong(toks_start[3]) + Long.parseLong(toks_start[5])
                            + Long.parseLong(toks_start[6]) + Long.parseLong(toks_start[7]) + Long.parseLong(toks_start[8]);
                long totaljiffies_start = Long.parseLong(toks_start[2]) + Long.parseLong(toks_start[3]) + Long.parseLong(toks_start[4]) + Long.parseLong(toks_start[5])
                               + Long.parseLong(toks_start[6]) + Long.parseLong(toks_start[7]) + Long.parseLong(toks_start[8]);

                Thread.sleep(100);

                reader.seek(0);
                load = reader.readLine();
                reader.close();

                String[] toks_end = load.split(" +");

                if (toks_end.length > 9)
                {
                    long workjiffies_end = Long.parseLong(toks_end[2]) + Long.parseLong(toks_end[3]) + Long.parseLong(toks_end[5])
                            + Long.parseLong(toks_end[6]) + Long.parseLong(toks_end[7]) + Long.parseLong(toks_end[8]);
                    long totaljiffies_end = Long.parseLong(toks_end[2]) + Long.parseLong(toks_end[3]) + Long.parseLong(toks_end[4]) + Long.parseLong(toks_end[5])
                            + Long.parseLong(toks_end[6]) + Long.parseLong(toks_end[7]) + Long.parseLong(toks_end[8]);
                    
                    if ((totaljiffies_end -totaljiffies_start) != 0)
                    {
                        cpuUsage = (workjiffies_end - workjiffies_start) * 100/(totaljiffies_end -totaljiffies_start);
                        if (cpuUsage > 99)
                            cpuUsage = 99;
                    }
                }
            }

            return String.valueOf(cpuUsage);
        } catch (Exception e) {
            e.printStackTrace();  
            return String.valueOf(cpuUsage);
        }
    }

    public String getMemInfo()  {
        long memTotal = 0;
        long memFree = 0;
        String memInfo="0:0";
        
        try {
            MemoryInfo mi = new MemoryInfo();
            @SuppressWarnings("static-access")
            ActivityManager activityManager = (ActivityManager) mContext.getSystemService(mContext.ACTIVITY_SERVICE);

            activityManager.getMemoryInfo(mi);
            memFree = mi.availMem / (1024* 1024);
            memTotal = mi.totalMem / (1024* 1024);
            
            memInfo = String.valueOf(memTotal) + ":" + String.valueOf(memFree);
        } catch (Exception e) {
             e.printStackTrace();  
        }

        return memInfo;
    }

    public String getDiskInfo()  {
        long diskTotal = 0;
        long diskFree = 0;
        String diskInfo="0:0";
        
        try {
            String dataPath = Environment.getDataDirectory().getAbsolutePath();
            StatFs statFs = new StatFs(dataPath);
            statFs.restat(dataPath);
            
            // In CTP 4.2.2, API 17, the long is 32 bit, so the value will be truncated.
            diskTotal = (statFs.getBlockCount()/1024) * (statFs.getBlockSize()/1024);
            diskFree  = (statFs.getAvailableBlocks()/1024) * (statFs.getBlockSize()/1024);
/*
            if (android.os.Build.VERSION.SDK_INT <= android.os.Build.VERSION_CODES.JELLY_BEAN_MR1) {
                diskTotal = (statFs.getBlockCount()/1024) * (statFs.getBlockSize()/1024);
                diskFree  = (statFs.getAvailableBlocks()/1024) * (statFs.getBlockSize()/1024);
            }else {
                diskFree  = statFs.getAvailableBytes() / (1024*1024);
                diskTotal  = statFs.getTotalBytes() / (1024*1024);
            }
*/            
              diskInfo = String.valueOf(diskTotal) + ":" + String.valueOf(diskFree);
        } catch (Exception e) {
             e.printStackTrace();  
        }

        return diskInfo;
    }

    public String getBatteryLevel()  {
        int batteryLevel = 0;
        String batteryLevelInfo="50";
        
        try {
            Intent batteryIntent = mContext.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
            int level = batteryIntent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
            int scale = batteryIntent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);

            if ((scale > 0) && (level > 0))
            {
                batteryLevel = (level  * 100 ) / scale;
                batteryLevelInfo = String.valueOf(batteryLevel) ;
            }
            
        } catch (Exception e) {
            e.printStackTrace();
        }
        return batteryLevelInfo;
    }

}