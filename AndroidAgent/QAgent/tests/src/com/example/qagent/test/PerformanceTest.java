package com.example.qagent.test;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import com.example.qagent.Performance;

public class PerformanceTest extends AndroidTestCase{
    private Performance perfTool;

    public PerformanceTest() {
    }

    @Override
    protected void setUp() throws Exception {
        perfTool = new Performance(getContext());
    }

    @Override
    protected void tearDown() throws Exception {
    }

    @MediumTest
    public void testCPUInfo() {
        String cpuusage;
        cpuusage = perfTool.getCpuUsage();
        assertNotNull("cpuusage is null!", cpuusage);
        assertTrue(Integer.parseInt(cpuusage) >= 0);
    }

    @MediumTest
    public void testMemoryInfo() {
        String meminfo;
        meminfo = perfTool.getMemInfo();
        assertNotNull("meminfo is null!", meminfo);
        assertTrue(!meminfo.equals("0:0"));
    }

    @MediumTest
    public void testDiskInfo() {
        String diskinfo;
        diskinfo = perfTool.getDiskInfo();
        assertNotNull("diskinfo is null!", diskinfo);
        assertTrue(!diskinfo.equals("0:0"));
    }

    @MediumTest
    public void testBatteryLevel() {
        String batteryLevel;
        batteryLevel = perfTool.getBatteryLevel();
        assertNotNull("batteryLevel is null!", batteryLevel);
        assertTrue(Integer.parseInt(batteryLevel) >= 0);
    }
}
