package com.example.qagent.test;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.Socket;
import java.util.Arrays;
import java.util.Vector;

import android.test.ActivityInstrumentationTestCase2;
import android.test.TouchUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.example.qagent.MainActivity;
import com.example.qagent.QAgentService;
import com.example.qagent.R;

public class QAgentServiceTest extends ActivityInstrumentationTestCase2<MainActivity> {
    public final String TAG = "QAgentServiceTest ";
    private Vector<String> checklist;
    private MainActivity mMainActivity;
    private Button mStartService;
    
    public QAgentServiceTest() {
        super(MainActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setActivityInitialTouchMode(true);
        mMainActivity = getActivity();        
        mStartService = (Button) mMainActivity.findViewById(R.id.button2);

        checklist = new Vector<String>();
        assertNotNull("Create checklist Failure!", checklist);
        
        checklist.add("test");
        checklist.add("getresolution");
        checklist.add("gethomepackage");
        checklist.add("launch:com.example.qagent");
        checklist.add("launch");//wrong command
        checklist.add("launch:com.example.packagenotexist"); //wrong command
        checklist.add("getlanguage");
        checklist.add("getactionbarheight");
        checklist.add("getdefaultorientation");
        checklist.add("getorientation");
        checklist.add("setorientation:0");
        checklist.add("setorientation:1");
        checklist.add("setorientation:2");
        checklist.add("setorientation:3");
        checklist.add("setorientation:4"); //wrong command.
        checklist.add("getqagentversion");
        
        checklist.add("showlotus");
        checklist.add("hidelotus");
        checklist.add("showlotus:0");
        checklist.add("hidelotus");
        checklist.add("showlotus:1");
        checklist.add("hidelotus");

        checklist.add("getosversion");
        checklist.add("getsdkversion");
        checklist.add("getsysprop:ro.build.version.release");
        checklist.add("getsysprop:ro.notexist.item"); //wrong command.
        checklist.add("getsysprop:"); //wrong command.
        checklist.add("imechecker:start");
        checklist.add("imechecker:gettext");
        
        checklist.add("shownavigationbar:landscape");
        checklist.add("hidenavigationbar");
        checklist.add("shownavigationbar:portrait");
        checklist.add("hidenavigationbar");
        checklist.add("shownavigationbar");
        checklist.add("hidenavigationbar");
        
        checklist.add("getrotationsensor");
        checklist.add("getperformance:cpuusage");
        checklist.add("getperformance:memory");
        checklist.add("getperformance:disk");
        checklist.add("getperformance:batterylevel");
        
        checklist.add("unknowncommand");
        checklist.add("exit");
    }

    protected void tearDown() throws Exception {
        mMainActivity.stopLotusActivity();
        mMainActivity.stopQAgentService();
        mMainActivity.finish();
        Thread.sleep(500);
    }
    
    public void testPreconditions() {
        assertNotNull("mStartService is null", mMainActivity);
        assertNotNull("mStartService is null", mStartService);
    }

    public void testServiceFeatures(){
//        String checkresult;
        Socket dataSocket = null;
        BufferedReader socketRecv = null;
        PrintWriter socketSend = null;
        char[] recvBuffer = new char[128];
        int SERVERPORT = 31700;
        String SERVER_IP = "127.0.0.1";

        try
        {
            TouchUtils.clickView(this, mStartService);
            assertTrue(View.VISIBLE == mStartService.getVisibility());

            InetAddress serverAddr = InetAddress.getByName(SERVER_IP);
            dataSocket = new Socket(serverAddr, SERVERPORT);
            if ((dataSocket != null) && (dataSocket.isConnected()))
            {
                socketRecv = new BufferedReader(new InputStreamReader(dataSocket.getInputStream()));
                socketSend = new PrintWriter(new BufferedWriter(new OutputStreamWriter(dataSocket.getOutputStream())), true);
        
                for (int i = 0; i < checklist.size(); i++)
                {
                    assertNotNull("QAgentService command is null!", checklist.get(i));
                    socketSend.write(checklist.get(i) + "\n"); // must end with "\n"
                    socketSend.flush();

                    Thread.sleep(200);
                    Arrays.fill(recvBuffer, '\n');
                    if (socketRecv.read(recvBuffer) > 0)
                    {
                        String tmpStr = new String(recvBuffer);
                          Log.i(TAG, "Get reply: " + tmpStr);
                        assertNotNull("Get reply buffer is null!", tmpStr);                          
//                      assertTrue("recvBuffer is null!", tmpStr.contains(checklist.get(i)));
                    }
                    Thread.sleep(1000);
                }

            }
         } catch (Exception e) {
              Log.e(TAG, " testServiceFeatures allocate socket error: " + e.toString());
         } 
    }
    
    public void testServiceConfigChange(){
        QAgentService qagentService = new QAgentService();
        assertNotNull("QAgentService is null!", qagentService);
        
        qagentService.initResource();
        qagentService.onConfigurationChanged(null);
        
        assertTrue("QAgentService configure changed!", qagentService.isConfigChanged);
    }    
}


