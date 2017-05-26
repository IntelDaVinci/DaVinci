package com.example.qagent;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

public class LotusActivity extends Activity {
    public final static String TAG = "QAgentLotusActivity ";
    static Activity currentObject = null;
    WindowManager winManager = null;   
    Display defaultDisplay = null;    
    private int defaultOrientation = Configuration.ORIENTATION_PORTRAIT;
    private int currentRotation = 0;
    private int expectedRotation = 0;
    protected PowerManager.WakeLock mWakeLock = null;
    
    @Override
    @SuppressWarnings("deprecation")
    protected void onCreate(Bundle savedInstanceState) {        
        super.onCreate(savedInstanceState);
        
        currentObject = this;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            this.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        } else {
            this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
        setContentView(R.layout.activity_lotus);

        winManager = (WindowManager) getBaseContext().getSystemService(Context.WINDOW_SERVICE);
        defaultDisplay= winManager.getDefaultDisplay();
        
        Configuration config = getResources().getConfiguration();
        currentRotation = defaultDisplay.getRotation();    

        if ( ((currentRotation == Surface.ROTATION_0 || currentRotation == Surface.ROTATION_180) &&
                config.orientation == Configuration.ORIENTATION_LANDSCAPE)
            || ((currentRotation == Surface.ROTATION_90 || currentRotation == Surface.ROTATION_270) &&    
                config.orientation == Configuration.ORIENTATION_PORTRAIT)) {
            defaultOrientation = Configuration.ORIENTATION_LANDSCAPE;
        } else { 
            defaultOrientation = Configuration.ORIENTATION_PORTRAIT;
        }

        if (this.getIntent().hasExtra("Orientation"))
        {
            expectedRotation = this.getIntent().getExtras().getShort("Orientation");
        }
        
        TextView bg1 = (TextView) findViewById(R.id.bg1);
        TextView bg2 = (TextView) findViewById(R.id.bg2);

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int screenWidth = dm.widthPixels;
        int screenHeight = dm.heightPixels;
        if (screenWidth > screenHeight) {
            bg1.setVisibility(TextView.VISIBLE);
            bg2.setVisibility(TextView.GONE);
        }
        
        Window win = getWindow();
        WindowManager.LayoutParams winParams = win.getAttributes();
        winParams.flags |= (WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD 
                | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED 
                | WindowManager.LayoutParams.FLAG_ALLOW_LOCK_WHILE_SCREEN_ON 
                | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
                | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); 
        win.setAttributes(winParams);

        //light the screen
        PowerManager pm = (PowerManager)getBaseContext().getSystemService(Context.POWER_SERVICE);
        mWakeLock= pm.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP, "Tag For Debug");

        if ((mWakeLock != null) && (!mWakeLock.isHeld()))
            mWakeLock.acquire();

        SharedPreferences prefs = getSharedPreferences ("com.android.launcher2.prefs", Context.MODE_PRIVATE);
        Log.i(TAG, " Current cling is " + prefs.getBoolean("cling.workspace.dismissed", false));
    }
    
    protected void onResume(){
        // full screen
        if ((mWakeLock != null) && (!mWakeLock.isHeld()))
            mWakeLock.acquire();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            this.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        } else {
            this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
           
        Log.i(TAG, " Received orientation is " + expectedRotation);
           
           if (defaultOrientation == Configuration.ORIENTATION_LANDSCAPE)
           {
               if (expectedRotation == 1)
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
               else if (expectedRotation == 2)
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
               else if(expectedRotation == 3)
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
               else
                   setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
           }
           else
           {
               if(expectedRotation == 1)
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
               else if (expectedRotation == 2)
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
               else if (expectedRotation == 3)
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
               else
                   setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
           }

           super.onResume();
    }
    
    @Override
    protected void onPause()
    {
        if ((mWakeLock != null) && (mWakeLock.isHeld()))
            mWakeLock.release();
        
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if ((mWakeLock != null) && (mWakeLock.isHeld()))
            mWakeLock.release();

        currentObject = null;
        super.onDestroy();
        Log.i(TAG, " LotusActivity destroyed!");
    }
}
