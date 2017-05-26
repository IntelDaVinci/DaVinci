package com.example.qagent;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;

public class RotationActivity extends Activity {
    public final static String TAG = "QAgentRotationActivity ";
    public static Activity currentObject = null;
    protected PowerManager.WakeLock mWakeLock = null;
    private int defaultOrientation = Configuration.ORIENTATION_PORTRAIT;
    private int expectedRotation = Configuration.ORIENTATION_PORTRAIT;
    private boolean autoExit = false;

    @Override
    @SuppressWarnings("deprecation")
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        currentObject = this;
        
        if (this.getIntent().hasExtra("ExpectedRotation"))
        {
            expectedRotation = this.getIntent().getExtras().getShort("ExpectedRotation");
        }

        if (this.getIntent().hasExtra("AutoExit"))
        {
            autoExit = this.getIntent().getExtras().getBoolean("AutoExit");
        }

        //light the screen
        PowerManager pm = (PowerManager)getBaseContext().getSystemService(Context.POWER_SERVICE);
        mWakeLock= pm.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP, "Tag For Debug");
        if ((mWakeLock != null) && (!mWakeLock.isHeld()))
            mWakeLock.acquire();
        
        WindowManager winManager = (WindowManager) getBaseContext().getSystemService(Context.WINDOW_SERVICE);   
        Display defaultDisplay = winManager.getDefaultDisplay();    
        Configuration config = getResources().getConfiguration();
        int orientation = defaultDisplay.getRotation();

        if ( ((orientation == Surface.ROTATION_0 || orientation == Surface.ROTATION_180) &&
                config.orientation == Configuration.ORIENTATION_LANDSCAPE)
            || ((orientation == Surface.ROTATION_90 || orientation == Surface.ROTATION_270) &&    
                config.orientation == Configuration.ORIENTATION_PORTRAIT)) {
            defaultOrientation = Configuration.ORIENTATION_LANDSCAPE;
        } else { 
            defaultOrientation = Configuration.ORIENTATION_PORTRAIT;
        }
    }

    public void onResume(){
        if ((mWakeLock != null) && (!mWakeLock.isHeld()))
            mWakeLock.acquire();
           
        try
        {
            Log.i(TAG, " Received rotation is " + expectedRotation + " AutoExit is " + autoExit);

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
            
            if (autoExit)
            {
                Thread.sleep(100);
                this.finish();
            }
        } catch (Exception e){
            Log.e(TAG, " Setrotation error: " + e.toString());
        }
    }
    
    @Override
    public void onPause()
    {
        if ((mWakeLock != null) && (mWakeLock.isHeld()))
            mWakeLock.release();

        super.onPause();
    }

    @Override
    public void onDestroy() {
        if ((mWakeLock != null) && (mWakeLock.isHeld()))
            mWakeLock.release();

        super.onDestroy();
        currentObject = null;
        Log.i(TAG, " RotationActivity destroyed!");
    }
}
