package com.example.qagent.test;

import com.example.qagent.RotationActivity;

import android.content.Intent;
import android.os.Bundle;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;
import android.view.Surface;

public class RotationActivityTest extends ActivityInstrumentationTestCase2<RotationActivity> {
    public final String TAG = "RotationActivityTest ";
    private RotationActivity mRotationActivity;
    
    public RotationActivityTest() {
        super(RotationActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setActivityInitialTouchMode(true);
        //Get a reference to the Activity under test, starting it if necessary.
    }
    
    @Override
    protected void tearDown() throws Exception {
    	mRotationActivity.finish();    	
        super.tearDown();
    }

    public void testSetOrientations() {
        try{
            setOrientations(Surface.ROTATION_0);
            Thread.sleep(1000);
            setOrientations(Surface.ROTATION_90);
            Thread.sleep(1000);
            setOrientations(Surface.ROTATION_180);
            Thread.sleep(1000);
            setOrientations(Surface.ROTATION_270);
            Thread.sleep(1000);
        } catch (Exception e) {
            Log.e(TAG, " Failed testSetOrientations " + e.toString());
        }
    }

    @MediumTest
    public void testPauseResume() {
        Intent intent = new Intent();
        Bundle bundle=new Bundle();  
        bundle.putShort("ExpectedRotation", (short) Surface.ROTATION_0);
        bundle.putBoolean("AutoExit", true);
        intent.putExtras(bundle);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        setActivityIntent(intent);

        mRotationActivity = getActivity();
        
        mRotationActivity.onResume();
        assertNotNull("mRotationActivity is null", mRotationActivity.getIntent());

        mRotationActivity.onPause();
        assertNotNull("mRotationActivity is null", mRotationActivity.getIntent());

        mRotationActivity.onDestroy();        
        assertNotNull("mRotationActivity is null", mRotationActivity.getIntent());
    }

    public void setOrientations(int rotation)  {
        try{
            Intent intent = new Intent();
            Bundle bundle = new Bundle();  
            bundle.putShort("ExpectedRotation", (short) rotation);
            bundle.putBoolean("AutoExit", true);
            intent.putExtras(bundle);
            Log.i(TAG, "Change rotation to " + rotation);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

            setActivityIntent(intent);
            mRotationActivity = getActivity();

            Thread.sleep(1000);

            for (int i=0; i<5; i++) 
            {
                if ((RotationActivity.currentObject != null) && (!RotationActivity.currentObject.isDestroyed()))
                {
                    Thread.sleep(1000);
                    Log.i(TAG, " RotationActivity still alive!");
                }
                else
                {
                    break;
                }
            }
            
            if (RotationActivity.currentObject != null)
            {
                RotationActivity.currentObject.finish();
                Log.i(TAG, " Kill RotationActivity again!");
            }

            RotationActivity.currentObject = null;
        } catch (Exception e) {
            Log.e(TAG, " Failed setorientattion " + e.toString());
        }
    }
}