package com.example.qagent;

import java.io.BufferedWriter;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.Socket;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.View;

public class MainActivity extends Activity {
    public final static String TAG = "QAgentMainActivity ";
    public static String STARTQAGENT = "com.example.startqagent";
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    public void showLotus(View view) {
        startActivity(new Intent(this,LotusActivity.class));
        Log.i(TAG, "Start lotus activity!");
    }
    
    public void startService(View view) {
        this.startService(new Intent(this, QAgentService.class));
        Log.i(TAG, "Start QAgentService!");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "MainActivity onDestroy!");
    }
    
    public void stopLotusActivity() {
        if (LotusActivity.currentObject != null)
        {
            LotusActivity.currentObject.finish();
        }

        LotusActivity.currentObject = null;
        Log.i(TAG, "Stop LotusActivity!");
    }
    
    public void stopQAgentService()
    {
        // Set QAgent main thread's running flag.
        if ((QAgentService.currentObject != null) && (QAgentService.runningFlag == true))
        {
            QAgentService.runningFlag = false;
        }

        Socket dataSocket = null;
        PrintWriter socketSend = null;
        int SERVERPORT = 31700;
        String SERVER_IP = "127.0.0.1";
        try
        {
           InetAddress serverAddr = InetAddress.getByName(SERVER_IP);
           dataSocket = new Socket(serverAddr, SERVERPORT);
           if ((dataSocket != null) && (dataSocket.isConnected()))
           {
               socketSend = new PrintWriter(new BufferedWriter(new OutputStreamWriter(dataSocket.getOutputStream())), true);
 
               socketSend.write("exit\n"); // must end with "\n"
               socketSend.flush();

               Thread.sleep(2000);
           }
        } catch (Exception e) {
            Log.w(TAG, " stopQAgentService error: " + e.toString());
        }
          
        stopService(new Intent(this, QAgentService.class));

        Log.i(TAG, "Stop QAgentService!");
    }
}
