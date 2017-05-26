package com.example.qagent.test;

import java.util.Vector;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import com.example.qagent.SystemProperty;

public class SystemPropertyTest extends AndroidTestCase{
    private SystemProperty props;
    private Vector<String> checklist;
    
    public SystemPropertyTest() {
    }

    @Override
    protected void setUp() throws Exception {
        props = new SystemProperty(getContext());
        assertNotNull("Create SystemProperty Failure!", props);
        checklist = new Vector<String>();
        assertNotNull("Create checklist Failure!", checklist);
        
        checklist.add("ro.build.version.release");
        checklist.add("ro.product.cpu.abi");
        checklist.add("ro.build.version.sdk");
        checklist.add("ro.product.locale.language");
        checklist.add("persist.sys.language");
        checklist.add("ro.product.manufacturer");
    }

    
    @MediumTest
    public void testPropertiyInfo() {
        String checkresult;

        for (int i = 0; i < checklist.size(); i++)
        {
            checkresult = props.get(checklist.get(i));
            assertNotNull("getprop is null!", checkresult);
        }
    }
}