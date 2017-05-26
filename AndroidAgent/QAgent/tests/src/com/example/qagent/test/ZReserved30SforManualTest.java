package com.example.qagent.test;

import com.example.qagent.MainActivity;

import android.test.ActivityInstrumentationTestCase2;
import android.test.TouchUtils;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import com.example.qagent.R;

public class ZReserved30SforManualTest extends ActivityInstrumentationTestCase2<MainActivity> {
    public final String TAG = "MainActivityTest ";
    private MainActivity mMainActivity;
    private Button mStartService;
    private Button mShowLotus;
    
    public ZReserved30SforManualTest() {
        super(MainActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setActivityInitialTouchMode(true);

        //Get a reference to the Activity under test, starting it if necessary.
        mMainActivity = getActivity();        
        mStartService = (Button) mMainActivity.findViewById(R.id.button2);
        mShowLotus = (Button) mMainActivity.findViewById(R.id.button1);
    }
    
    protected void tearDown() throws Exception {
        mMainActivity.finish();
        Thread.sleep(500);
    }
    
    public void testPreconditions() {
        assertNotNull("mMainActivity is null", mMainActivity);
        assertNotNull("mStartService is null", mStartService);
        assertNotNull("mShowLotus is null", mShowLotus);
    }
    
    @MediumTest
    public void testReserved30SForManualBehaviorsCheck() {
        try
        {
            TouchUtils.clickView(this, mStartService);
            
            assertTrue(View.VISIBLE == mStartService.getVisibility());
            
            Thread.sleep(30000);
        } catch (Exception e) {
            Log.e(TAG, " testReserved30SForManualBehaviorsCheck: " + e.toString());
        } 
    }    
}