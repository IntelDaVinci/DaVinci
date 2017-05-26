package com.example.qagent;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Locale;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.IntentService;
import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;

public class QAgentService extends IntentService implements 
    SensorEventListener    {
    public final String TAG = "QAgentService ";
    public static int QAGENT_PORT = 31700;
    public static int QAGENT_VERSION = 30;
    public static int PERFORMANCEPERIOD = 20000; //20S
    static IntentService currentObject = null;
    static Boolean runningFlag = true;
    public boolean isConfigChanged = false;

    ServerSocket listenSocket = null;
    Socket acceptDataSocket = null;
    Socket dataSocket = null;
    BufferedReader socketRecv = null;
    PrintWriter socketSend = null;
    private ReadWriteLock dataSocketLock = null;

    WindowManager winManager = null;   
    Display defaultDisplay = null;
    DisplayMetrics displayMetrics = null;
    DisplayMetrics realDisplayMetrics = null;
    int realWidth = 0, realHeight = 0;
    int defaultDisplayMode = Configuration.ORIENTATION_UNDEFINED;
    
    PowerManager.WakeLock mWakeLock = null;
    PowerManager pwrManager = null;
    Performance perfTool = null;
    SystemProperty systemProps = null;
    
    Thread performanceThread = null;
    boolean perfThreadRunningFlag = true; 
    Semaphore perfSemaphore = null;
    
    boolean sensorListenerRunningFlag = false;
    private SensorManager mSensorManager;
    private Sensor rotationVectorSensor;
    private Sensor accelerometerSensor;
    private Sensor gravitySensor;
    private Sensor gyroscopeSensor;
    private Sensor linearAccelerometerSensor;
    private Sensor pressureSensor;
    private Sensor temperatureSensor;
    
    private float[] accelerometerDataArray = null;
    private float[] gravityDataArray = null;
    private float[] gyroscopeDataArray = null;
    private float[] linearAccelerometerDataArray = null;
    private float[] rotationVectorDataArray = null;
    private float[] pressureDataArray = null;
    private float[] temperatureDataArray = null;

    public QAgentService() {
        super("");
    }

    @TargetApi(Build.VERSION_CODES.GINGERBREAD)
    @SuppressWarnings("deprecation")
    public boolean initResource() {
        try {
            runningFlag = true;
            currentObject = this;
            perfSemaphore = new Semaphore(1, true);

            winManager = (WindowManager) getBaseContext().getSystemService(Context.WINDOW_SERVICE);
            pwrManager = (PowerManager) getBaseContext().getSystemService(Context.POWER_SERVICE);
            
            mWakeLock = pwrManager.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP, "Tag For Debug");
            defaultDisplay= winManager.getDefaultDisplay();
            displayMetrics = new DisplayMetrics();
            realDisplayMetrics = new DisplayMetrics();
            perfTool = new Performance(getBaseContext());
            systemProps = new SystemProperty(getBaseContext());
            
            mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
            
            if ((winManager == null) || (pwrManager == null) || (mSensorManager == null) || (mWakeLock == null) 
                || (defaultDisplay == null) || (displayMetrics == null) || (realDisplayMetrics == null) 
                || (perfTool == null) || (systemProps == null))
            {
                Log.e(TAG, " Init system resource failure!");
                return false;
            }
                
            defaultDisplay.getMetrics(displayMetrics);
            defaultDisplay.getRealMetrics(realDisplayMetrics);

            int screenWidth = (int) (displayMetrics.widthPixels * displayMetrics.density);
            int screenHeight = (int) (displayMetrics.heightPixels * displayMetrics.density);
            Method mGetRawH = null, mGetRawW = null;

            Configuration config = getResources().getConfiguration();
            int orientation = defaultDisplay.getRotation();

            if ( ((orientation == Surface.ROTATION_0 || orientation == Surface.ROTATION_180) &&
                    config.orientation == Configuration.ORIENTATION_LANDSCAPE)
                || ((orientation == Surface.ROTATION_90 || orientation == Surface.ROTATION_270) &&    
                    config.orientation == Configuration.ORIENTATION_PORTRAIT)) {
                defaultDisplayMode = Configuration.ORIENTATION_LANDSCAPE;
            } else { 
                defaultDisplayMode = Configuration.ORIENTATION_PORTRAIT;
            }

            realWidth = screenWidth;
            realHeight = screenHeight;

            // For JellyBeans and onward
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN_MR1) {
                realWidth = realDisplayMetrics.widthPixels;
                realHeight = realDisplayMetrics.heightPixels;
            } else {
                // Below Jellybeans you can use reflection method
                mGetRawH = Display.class.getMethod("getRawHeight");
                mGetRawW = Display.class.getMethod("getRawWidth");

                realWidth = (Integer) mGetRawW.invoke(defaultDisplay);
                realHeight = (Integer) mGetRawH.invoke(defaultDisplay);
            }

            if (realHeight < realWidth) {
                int temp = realHeight;
                realHeight = realWidth;
                realWidth = temp;
            }
            
            if (!initServerSocket())
            {
                Log.e(TAG, " initServerSocket failure!");
                return false;
            }

            if ((mWakeLock != null) && (!mWakeLock.isHeld()))
                mWakeLock.acquire();
        
            initSensorListener();
            
            startPerformanceThread();
        } catch (Exception e) {
            Log.e(TAG, " Service resource init failure: " + e.toString());
            return false;
        }
        return true;
    }
    
    private void releaseResource()
    {
        stopPerformanceThread();
        releaseDataSocket();
        if ((mWakeLock != null) && (mWakeLock.isHeld()))
            mWakeLock.release();
    }

    private boolean initServerSocket()
    {
        try
        {
            dataSocketLock = new ReentrantReadWriteLock();
            listenSocket = new ServerSocket(QAgentService.QAGENT_PORT);
            
            Log.i(TAG, " The Server is listening at port " + QAgentService.QAGENT_PORT + "...");
        } catch (Exception e) {
            Log.e(TAG, " Init listen Socket error: " + e.toString());
            return false;
        } 
        return true;
    }

    private boolean initDataSocket()
    {
        try {
            acceptDataSocket = listenSocket.accept();

            dataSocketLock.writeLock().lock();
            dataSocket = acceptDataSocket;
            dataSocket.setTcpNoDelay(true);
            Log.i(TAG, " Socket accepted.");
            socketRecv = new BufferedReader(new InputStreamReader(dataSocket.getInputStream()));
            socketSend = new PrintWriter(new BufferedWriter(new OutputStreamWriter(dataSocket.getOutputStream())), true);
            dataSocketLock.writeLock().unlock();
         } catch (Exception e) {
            Log.e(TAG, " initDataSocket exception: " + e.toString());
            return false;
        }
        return true;
    }

    private void sendToDaVinci(String inputStr)
    {
        if ((dataSocketLock == null) || (dataSocket == null) || (inputStr == null))
            return;
        
        try {
            if (dataSocket.isConnected())
            {
                dataSocketLock.writeLock().lock();
                socketSend.print(inputStr);
                socketSend.flush();
                dataSocketLock.writeLock().unlock();
                Log.i(TAG, " Reply with " + inputStr);
            }
        } catch (Exception e){
            Log.e(TAG, " Socket send error: " + e.toString());
        }
        if (dataSocketLock.writeLock().tryLock())
            dataSocketLock.writeLock().unlock();
    }

    private void releaseDataSocket()
    {
        try {
            dataSocketLock.writeLock().lock();
            if (dataSocket != null)
            {
                if (socketRecv != null)
                    socketRecv.close();
                if (socketSend != null)
                    socketSend.close();

                dataSocket.close();
            }
            dataSocketLock.writeLock().unlock();
         } catch (Exception e) {
            Log.e(TAG, " Close socket exception: " + e.toString());
        }

        if (dataSocketLock.writeLock().tryLock())
            dataSocketLock.writeLock().unlock();
    }

    @SuppressWarnings("deprecation")
    @Override
    protected void onHandleIntent(Intent workIntent) {
        // Make sure the service will always running backend.
        Notification notification = new Notification(R.drawable.ic_launcher, "QAgentservice is Running", System.currentTimeMillis()); 
        notification.setLatestEventInfo(this, "QAgentservice", "QAgentservice is Running", PendingIntent.getService(this, 0, workIntent, 0)); 
        startForeground(1, notification);

        if (!initResource())
        {
            Log.e(TAG, " Service failed to init system resource!");
            return;
        }

        while (runningFlag)
        {
            try
            {
                if (!initDataSocket())
                {
                    Thread.sleep(1000);
                    continue;
                }
                
                while (true) {
                    String requestStr = socketRecv.readLine();
                    if (requestStr.equals("exit")) {
                        Log.i(TAG, " Received Exit request.");
                        break;                    
                    }

                    String replyStr = msgHandler(requestStr);
                    
                    sendToDaVinci(replyStr);
                }
            } catch (Exception e) {
                Log.e(TAG, " Socket receive error: " + e.toString());
            } 
            releaseDataSocket();
        }

        releaseResource();
    }
    
    @Override
    public void onDestroy() {
        runningFlag = false;

        releaseResource();
        super.onDestroy();
        currentObject = null;
    }

    @SuppressLint("InlinedApi")
    public String msgHandler(String inputStr)
    {
        String replyStr = null;

        if (inputStr.equals("test")) {
            replyStr = "auto:test";
        }
        else if (inputStr.equals("getresolution"))
        {
            replyStr = inputStr + ":" + String.valueOf(realHeight) + ":" + String.valueOf(realWidth);
        }
        else if (inputStr.equals("gethomepackage"))
        {
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.addCategory(Intent.CATEGORY_HOME);
            ResolveInfo resolveInfo = getPackageManager().resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
            String currentHomePackage = resolveInfo.activityInfo.packageName;
            replyStr = inputStr + ":" + currentHomePackage;
        }
        else if (inputStr.startsWith("launch"))
        {
            replyStr = inputStr + ":" + "failed";

            String[] items = inputStr.split(":");
            if(items.length > 1)
            {
                PackageManager packageManager = getPackageManager();
                Intent intent = packageManager.getLaunchIntentForPackage(items[1]);
                if(intent != null)
                    replyStr = "launch" + ":" + intent.getComponent().getClassName();
            }            
        }
        else if (inputStr.equals("getlanguage"))
        {
            replyStr = inputStr + ":" + "en";
            String sysLanguage = Locale.getDefault().getLanguage();
            if(sysLanguage != null)
                replyStr = inputStr + ":" + sysLanguage;
        }
        else if (inputStr.equals("getactionbarheight"))
        {
            TypedValue tv = new TypedValue();
            int actionBarHeight = 0;
            if (getTheme().resolveAttribute(android.R.attr.actionBarSize, tv, true))
            {
                actionBarHeight = TypedValue.complexToDimensionPixelSize(tv.data,getResources().getDisplayMetrics());
            }
            replyStr = inputStr + ":" + String.valueOf(actionBarHeight);
        }
        else if (inputStr.equals("getdefaultorientation"))
        {
            if (defaultDisplayMode == Configuration.ORIENTATION_LANDSCAPE)
                replyStr = inputStr + ":" + "1";
            else 
                replyStr = inputStr + ":" + "0";
        }
        else if (inputStr.equals("getorientation"))
        {
            String currentOrientStr = getSystemOrientation();
            replyStr = inputStr + ":" + currentOrientStr;
        }
        else if (inputStr.startsWith("setorientation"))
        {
            replyStr = setOrientation(inputStr);
        }
        else if (inputStr.equals("getqagentversion"))
        {
            replyStr = inputStr + ":" +  String.valueOf(QAGENT_VERSION);
        }
        else if (inputStr.equals("hidelotus"))
        {
            replyStr = hideLotus(inputStr);
        }
        else if (inputStr.startsWith("showlotus"))
        {
            replyStr = showLotus(inputStr);
        }
        else if (inputStr.equals("getosversion"))
        {
            String osVersion = "4.4"; 
            osVersion = android.os.Build.VERSION.RELEASE;
            replyStr = inputStr + ":" + osVersion;
        }
        else if (inputStr.equals("getsdkversion"))
        {
            int sdkVersion = 18;
            sdkVersion = android.os.Build.VERSION.SDK_INT;
            replyStr = inputStr + ":" + String.valueOf(sdkVersion);
        } 
        else if (inputStr.startsWith("getsysprop:"))
        {
            replyStr = getSysprop(inputStr);
        }
        else if(inputStr.startsWith("shownavigationbar"))
        {
           replyStr = showNavigationbar(inputStr);
        }
        else if(inputStr.startsWith("hidenavigationbar"))
        {
            replyStr = hideNavigationbar(inputStr);
        }
        else if (inputStr.startsWith("getperformance"))
        {
            replyStr = getPerformanceData(inputStr);            
        }
        else if (inputStr.startsWith("getsensordata"))
        {
            replyStr = getSensorData(inputStr);
        }
        else
        {
            Log.e(TAG, " Unknown command: " + inputStr);
            replyStr= inputStr + ":" + "Unknown";
        }
        
        return replyStr;
    }
    
    private String getSystemOrientation()
    {
        String currentOrientation = "0";

        if (defaultDisplay == null) 
            return currentOrientation;

        int orientation = defaultDisplay.getRotation();
        
        if (defaultDisplayMode == Configuration.ORIENTATION_LANDSCAPE)
        {
            if(orientation == Surface.ROTATION_0)//landscape
            {
                currentOrientation = "1";
            }
            else if (orientation == Surface.ROTATION_90)//reverse portrait
            {
                currentOrientation = "2";
            }
            else if(orientation == Surface.ROTATION_180)//reverse landscape
            {
                currentOrientation = "3";
            }
            else if (orientation == Surface.ROTATION_270)//portrait
            {
                currentOrientation = "0";
            }
        }
        else
        {
            if(orientation == Surface.ROTATION_0)//portrait
            {
                currentOrientation = "0";
            }
            else if (orientation == Surface.ROTATION_90)//landscape
            {
                currentOrientation = "1";
            }
            else if(orientation == Surface.ROTATION_180)//reverse portrait
            {
                currentOrientation = "2";
            }
            else if (orientation == Surface.ROTATION_270)//reverse landscape
            {
                currentOrientation = "3";
            }
        }

        return currentOrientation;
    }
    
    private int getRotationData(String orientation)
    {
        int rotation =  Surface.ROTATION_0;
        
        if (defaultDisplayMode == Configuration.ORIENTATION_LANDSCAPE)
        {
            if (orientation.equals("1"))
            {
                rotation = Surface.ROTATION_0;
            }
            else if (orientation.equals("2"))//reverse portrait
            {
                rotation = Surface.ROTATION_90;
            }
            else if (orientation.equals("3"))//reverse landscape
            {
                rotation = Surface.ROTATION_180;
            }
            else if (orientation.equals("0"))//portrait
            {
                rotation = Surface.ROTATION_270;
            }
        }
        else
        {
            if (orientation.equals("0"))//portrait
            {
                rotation = Surface.ROTATION_0;
            }
            else if(orientation.equals("1"))//landscape
            {
                rotation = Surface.ROTATION_90;
            }
            else if (orientation.equals("2"))//reverse portrait
            {
                rotation = Surface.ROTATION_180;
            }
            else if (orientation.equals("3"))//reverse landscape
            {
                rotation = Surface.ROTATION_270;
            }
        }

        return rotation;
    }

    private void initSensorListener()
    {
        accelerometerDataArray = new float[3];
        gravityDataArray = new float[3];
        gyroscopeDataArray = new float[3];
        linearAccelerometerDataArray = new float[3];
        rotationVectorDataArray = new float[3];
        pressureDataArray = new float[3];
        temperatureDataArray = new float[3];

        if (mSensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR) != null)
            rotationVectorSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
        else
            Log.e(TAG, "Didn't detect TYPE_ROTATION_VECTOR sensor!");

        if (mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER) != null)
            accelerometerSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        else
            Log.e(TAG, "Didn't detect TYPE_ACCELEROMETER sensor!");

        if (mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY) != null)
            gravitySensor = mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);
        else
            Log.e(TAG, "Didn't detect TYPE_GRAVITY sensor!");

        if (mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE) != null)
            gyroscopeSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        else
            Log.e(TAG, "Didn't detect TYPE_GYROSCOPE sensor!");

        if (mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION) != null)
            linearAccelerometerSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
        else
            Log.e(TAG, "Didn't detect TYPE_LINEAR_ACCELERATION sensor!");

        if (mSensorManager.getDefaultSensor(Sensor.TYPE_PRESSURE) != null)
            pressureSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_PRESSURE);
        else
            Log.e(TAG, "Didn't detect TYPE_PRESSURE sensor!");

        if (mSensorManager.getDefaultSensor(Sensor.TYPE_AMBIENT_TEMPERATURE) != null)
            temperatureSensor  = mSensorManager.getDefaultSensor(Sensor.TYPE_AMBIENT_TEMPERATURE);
        else
            Log.e(TAG, "Didn't detect TYPE_AMBIENT_TEMPERATURE sensor!");
    }
    
    public boolean startSensorListener() {
        try
        {
            if (sensorListenerRunningFlag == false)
            {
                Thread startSensorListenerThread = new Thread() {
                    @Override
                    public void run() {
                        if (currentObject != null)
                        {
                            if (rotationVectorSensor != null)
                                mSensorManager.registerListener((SensorEventListener) currentObject, rotationVectorSensor, SensorManager.SENSOR_DELAY_UI);
                            if (accelerometerSensor != null)
                                mSensorManager.registerListener((SensorEventListener) currentObject, accelerometerSensor, SensorManager.SENSOR_DELAY_UI);
                            if (gravitySensor != null)
                                mSensorManager.registerListener((SensorEventListener) currentObject, gravitySensor, SensorManager.SENSOR_DELAY_UI);
                            if (gyroscopeSensor != null)
                                mSensorManager.registerListener((SensorEventListener) currentObject, gyroscopeSensor, SensorManager.SENSOR_DELAY_UI);
                            if (linearAccelerometerSensor != null)
                                mSensorManager.registerListener((SensorEventListener) currentObject, linearAccelerometerSensor, SensorManager.SENSOR_DELAY_UI);
                            if (pressureSensor != null)
                                mSensorManager.registerListener((SensorEventListener) currentObject, pressureSensor, SensorManager.SENSOR_DELAY_UI);
                            if (temperatureSensor != null)
                                mSensorManager.registerListener((SensorEventListener) currentObject, temperatureSensor, SensorManager.SENSOR_DELAY_UI);

                            sensorListenerRunningFlag = true;
                        }
                   }
                };

                if ((startSensorListenerThread != null))
                    startSensorListenerThread.start();
            }
        } catch (Exception e) {
            Log.e(TAG, "startSensorListener exception: " + e.toString());
            return false;
        }

        return true;
    }

    public boolean stopSensorListener() {
        try
        {
            if (sensorListenerRunningFlag == true)
            {
                sensorListenerRunningFlag = false;
                Thread stopSensorListenerThread = new Thread() {
                    @Override
                    public void run() {
                        if (currentObject != null)
                        {
                            if (rotationVectorSensor != null)
                                mSensorManager.unregisterListener((SensorEventListener) currentObject, rotationVectorSensor);
                            if (accelerometerSensor != null)
                                mSensorManager.unregisterListener((SensorEventListener) currentObject, accelerometerSensor);
                            if (gravitySensor != null)
                                mSensorManager.unregisterListener((SensorEventListener) currentObject, gravitySensor);
                            if (gyroscopeSensor != null)
                                mSensorManager.unregisterListener((SensorEventListener) currentObject, gyroscopeSensor);
                            if (linearAccelerometerSensor != null)
                                mSensorManager.unregisterListener((SensorEventListener) currentObject, linearAccelerometerSensor);
                            if (pressureSensor != null)
                                mSensorManager.unregisterListener((SensorEventListener) currentObject, pressureSensor);
                            if (temperatureSensor != null)
                                mSensorManager.unregisterListener((SensorEventListener) currentObject, temperatureSensor);
                        }
                   }
                };

                if ((stopSensorListenerThread != null))
                    stopSensorListenerThread.start();
            }
        } catch (Exception e) {
            Log.e(TAG, "stopSensorListener exception: " + e.toString());
            return false;
        }
        
        return true;
    }

    private String getSensorData(String inputStr)
    {
        String[] items = inputStr.split(":");
        String replyStr = inputStr;
        
        if(items.length > 1)
        {
            if(items[1].equals("start")){
                if (startSensorListener())
                    replyStr = inputStr + ":" + "succeed";
                else
                    replyStr = inputStr + ":" + "fail";

                return replyStr;
            }
           
            if(items[1].equals("stop")){
                if (stopSensorListener())
                    replyStr = inputStr + ":" + "succeed";
                else
                    replyStr = inputStr + ":" + "fail";

                return replyStr;
            }
            
            if (sensorListenerRunningFlag)
            {
                if (items[1].equals("accelerometer")){
                    replyStr = replyStr + ":" + accelerometerDataArray[0] + ":" + accelerometerDataArray [1] + ":" + accelerometerDataArray[2];
                }
                else if(items[1].equals("gyroscope")){
                    replyStr = inputStr + ":" + gyroscopeDataArray[0] + ":" + gyroscopeDataArray [1] + ":" + gyroscopeDataArray[2];
                }
                else if(items[1].equals("temperature")){
                    replyStr = replyStr + ":" + temperatureDataArray[0] + ":" + temperatureDataArray [1] + ":" + temperatureDataArray[2];
                }
                else if(items[1].equals("pressure")){
                    replyStr = replyStr + ":" + pressureDataArray[0] + ":" + pressureDataArray [1] + ":" + pressureDataArray[2];
                }
                else if(items[1].equals("linearacceleration")){
                    replyStr = replyStr + ":" + linearAccelerometerDataArray[0] + ":" + linearAccelerometerDataArray [1] + ":" + linearAccelerometerDataArray[2];
                }
                else if(items[1].equals("gravity")){
                    replyStr = inputStr + ":" + gravityDataArray[0] + ":" + gravityDataArray [1] + ":" + gravityDataArray[2];
                }
                else if(items[1].equals("rotationvector")){
                    replyStr = inputStr + ":" + rotationVectorDataArray[0] + ":" + rotationVectorDataArray [1] + ":" + rotationVectorDataArray[2];
                }
                else
                {
                    replyStr= inputStr + ":" + "Unknown";
                    Log.e(TAG, " Unknown command: " + inputStr);
                }
            }
            else
            {
                replyStr= inputStr + ":" + "Unknown";
                Log.e(TAG, " Sensor Listener Not Started for support command: " + inputStr);
            }
        }
        else
        {
            replyStr= inputStr + ":" + "Unknown";
            Log.e(TAG, " Unknown command: " + inputStr);
        }
        
        return replyStr;    
    }

    private void determineOrientation(float[] rotationMatrix)
    {
        float[] orientationValues = new float[3];
        SensorManager.getOrientation(rotationMatrix, orientationValues);
        rotationVectorDataArray[0] = (float) Math.toDegrees(orientationValues[0]); // Azimuth
        rotationVectorDataArray[1] = (float) Math.toDegrees(orientationValues[1]); // Pitch
        rotationVectorDataArray[2] = (float) Math.toDegrees(orientationValues[2]); // Roll
    }
    
    @Override
    public void onSensorChanged(SensorEvent event) {
        
        switch(event.sensor.getType()){
            case Sensor.TYPE_ROTATION_VECTOR:
                // All values are in degrees
                float[] rotationMatrix = new float[16];;
                SensorManager.getRotationMatrixFromVector(rotationMatrix, event.values);
                determineOrientation(rotationMatrix);
                break;
            case Sensor.TYPE_ACCELEROMETER:
                // All values are in SI units (m/s^2)
                // values[0]: Acceleration minus Gx on the x-axis
                // values[1]: Acceleration minus Gy on the y-axis
                // values[2]: Acceleration minus Gz on the z-axis
                accelerometerDataArray = event.values;
                break;
            case Sensor.TYPE_GRAVITY:
                // All values are in SI units (m/s^2)
                // values[0]: Gx on the x-axis
                // values[1]: Gy on the y-axis
                // values[2]: Gz on the z-axis
                gravityDataArray = event.values;
                break;
            case Sensor.TYPE_GYROSCOPE:
                // All values are in radians/second and measure the rate of rotation around the device's local X, Y and Z axis.
                // values[0]: Angular speed around the x-axis
                // values[1]: Angular speed around the y-axis
                // values[2]: Angular speed around the z-axis
                gyroscopeDataArray = event.values;
                break;
            case Sensor.TYPE_LINEAR_ACCELERATION:
                // A three dimensional vector indicating acceleration along each device axis, not including gravity. All values have units of m/s^2. 
                // values[0]: Acceleration on the x-axis
                // values[1]: Acceleration on the y-axis
                // values[2]: Acceleration on the z-axis
                linearAccelerometerDataArray = event.values;
                break;
            case Sensor.TYPE_PRESSURE:
                // values[0]: Atmospheric pressure in hPa (millibar) 
                pressureDataArray[0] = event.values[0];
                break;
            case Sensor.TYPE_AMBIENT_TEMPERATURE:
                //values[0]: ambient (room) temperature in degree Celsius. 
                temperatureDataArray[0] = event.values[0];
                break;
            default:
                break;
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
            String replyStr = null;
            String currentOrientStr = getSystemOrientation();

            replyStr = "auto:orientation:" + currentOrientStr;
            isConfigChanged = true;
            //sendToDaVinci(replyStr);
            Log.i(TAG, " Configure changed, send auto orientation event:" + replyStr);
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // TODO Auto-generated method stub
        
    }
    
    public Thread startPerformanceThread() {
        perfThreadRunningFlag = true;
        
        if ((performanceThread == null) || (!performanceThread.isAlive()))
        {
            performanceThread = new Thread() {
                @Override
                public void run() {
                    try {
                        String replyStr = null;
                        Performance perfTool = new Performance(getBaseContext());

                        String cpuusage;
                        String meminfo;
                        String batteryLevel;
                        String diskinfo;

                        while(perfThreadRunningFlag) {
                            perfSemaphore.tryAcquire(PERFORMANCEPERIOD, TimeUnit.MILLISECONDS);
                            cpuusage = perfTool.getCpuUsage();
                            meminfo = perfTool.getMemInfo();
                            batteryLevel = perfTool.getBatteryLevel();
                            diskinfo = perfTool.getDiskInfo();
                            
                            replyStr = "auto:performance:"  + cpuusage + ":" + meminfo + ":" + batteryLevel + ":" + diskinfo;

                            sendToDaVinci(replyStr);
                        }
                    } 
                    catch (Exception e) {
                        Log.e(TAG, " Create performance thread failure, " + e.toString());
                    }            
               }
            };

            if ((performanceThread != null))
                performanceThread.start();
        }
        
        return performanceThread;
    }

    public void stopPerformanceThread() {
        if ((performanceThread != null) && (performanceThread.isAlive()))
        {
            try 
            {
                perfThreadRunningFlag = false;
                perfSemaphore.release();
                performanceThread.join(1000);
            } catch (Exception e) {
                Log.e(TAG, " Close performance thread failure, " + e.toString());
            } 
        }
    }

    private String getPerformanceData(String inputStr)
    {
        String[] items = inputStr.split(":");
        String replyStr = inputStr + ":";
        
        if(items.length > 1)
        {
            if(items[1].equals("cpuusage")){
                String cpuusage = perfTool.getCpuUsage();
                replyStr = replyStr + cpuusage ;
            }
            else if(items[1].equals("memory")){
                String meminfo = perfTool.getMemInfo();
                replyStr = replyStr + meminfo ;
            }
            else if(items[1].equals("disk")){
                String diskInfo = perfTool.getDiskInfo();
                replyStr = replyStr + diskInfo;
            }
            else if(items[1].equals("batterylevel")){
                String batteryLevel = perfTool.getBatteryLevel();
                replyStr = replyStr + batteryLevel;
            }
        }
        else
        {
            replyStr= inputStr + "Unknown";
            Log.e(TAG, " Unknown command: " + inputStr);
        }
        
        return replyStr;
    }
    
    private String setOrientation(String inputStr)
    {
        int rotation = defaultDisplay.getRotation();
        String[] items = inputStr.split(":");
        String replyStr = inputStr;

        try
        {
            if(items.length > 1)
            {
                rotation = getRotationData(items[1]);
            }

            Intent intent = new Intent(this,RotationActivity.class);
            Bundle bundle=new Bundle();  
            bundle.putShort("ExpectedRotation", (short) rotation);
            bundle.putBoolean("AutoExit",true);
            intent.putExtras(bundle);
            Log.i(TAG, "Change rotation to " + rotation);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_TASK_ON_HOME);

            startActivity(intent);

            Thread.sleep(100);

            for (int i=0; i<5; i++) 
            {
                if ((RotationActivity.currentObject != null) && (!RotationActivity.currentObject.isDestroyed()))
                {
                    Thread.sleep(100);
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
            replyStr = inputStr + ":" + "succeed";
        } catch (Exception e) {
            Log.e(TAG, " Failed setorientattion " + e.toString());
            replyStr = inputStr + ":" + "fail";
        }
        
        return replyStr;
    }
    
    private String showNavigationbar(String inputStr)
    {
        String[] items = inputStr.split(":");
        int rotation = defaultDisplay.getRotation();
        String replyStr = inputStr;
        
        try
        {
            if(items.length > 1)
            {
                if(items[1].equals("landscape")){
                    if (defaultDisplayMode == Configuration.ORIENTATION_LANDSCAPE)
                    {
                        rotation = Surface.ROTATION_0;
                    }
                    else
                    {
                         rotation = Surface.ROTATION_90;
                    }
                }
                else
                {
                    if (defaultDisplayMode == Configuration.ORIENTATION_LANDSCAPE)
                    {
                        rotation = Surface.ROTATION_270;
                    }
                    else
                    {
                        rotation = Surface.ROTATION_0;
                    }
                }
            }
            else
            {
                rotation = Surface.ROTATION_0;
            }

            Log.i(TAG, " Display white screen in rotation:" + rotation);
            Intent intent = new Intent(this, RotationActivity.class);
            Bundle bundle = new Bundle();  
            bundle.putShort("ExpectedRotation", (short) rotation);
            bundle.putBoolean("AutoExit", false);
            intent.putExtras(bundle);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_TASK_ON_HOME);
            startActivity(intent);

            for (int i = 0; i < 3; i ++) 
            {
                if (RotationActivity.currentObject == null) 
                {
                    Thread.sleep(100);
                    Log.i(TAG, " RotationActivity still not create yet!");
                }
                else
                {
                    Log.i(TAG, " RotationActivity is alive!");
                    break;
                }
            }
            
            replyStr = inputStr + ":" + "succeed";
        } catch (Exception e) {
            Log.e(TAG, " Failed shownavigationbar " + e.toString());
            replyStr = inputStr + ":" + "fail";
        }

        return replyStr;
    }
    
    private String hideNavigationbar(String inputStr)
    {
        String replyStr = inputStr;

        try
        {
            Log.i(TAG, " hidenavigationbar.");

            for (int i = 0; i < 3; i ++) 
            {
                if ((RotationActivity.currentObject != null) && (!RotationActivity.currentObject.isDestroyed()))
                {
                    RotationActivity.currentObject.finish();
                    Thread.sleep(100);
                    Log.i(TAG, " Kill RotationActivity!");
                }
                else
                {
                    break;
                }
            }
            RotationActivity.currentObject = null;            
            replyStr = inputStr + ":" + "succeed";
        } catch (Exception e) {
            Log.e(TAG, " Failed to hidenavigationbar " + e.toString());
            replyStr = inputStr + ":" + "fail";
        }

        return replyStr;
    }
    
    private String showLotus(String inputStr)
    {
        String replyStr = inputStr;
        short orientation = 0;
        
        try
        {
            String[] items = inputStr.split(":");

            if ((items.length > 1) && (items[1].length() == 1)) 
            {
                orientation = Short.parseShort(items[1]);
            }

            Log.i(TAG, " Showlotus orientation is: " + orientation);
            Intent intent = new Intent(this, LotusActivity.class);
            Bundle bundle = new Bundle();  
            bundle.putShort("Orientation", orientation);
            intent.putExtras(bundle);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
            replyStr = inputStr + ":" + "succeed";
        } catch (Exception e) {
            Log.i(TAG, " Showlotus failure: " + e.toString());
            replyStr = inputStr + ":" + "fail";
        }

        return replyStr;
    }
    
    private String hideLotus(String inputStr)
    {
        String replyStr = inputStr;

        Log.i(TAG, " Hide lotus");
        if (LotusActivity.currentObject != null)
        {
            LotusActivity.currentObject.finish();
        }
        LotusActivity.currentObject = null;
        replyStr = inputStr + ":" + "succeed";
        
        return replyStr;
    }
    
    private String getSysprop(String inputStr)
    {
        String replyStr = inputStr;
        String[] items = inputStr.split(":");
        String sysprop = "";
        
        if ((items.length > 1) && (items[1].length() > 1)) 
        {
            sysprop = systemProps.get(items[1]);
            replyStr = inputStr + ":" + sysprop;
        }
        else
        {
            replyStr = inputStr + ":" + "Unknown";
        }

        return replyStr;
    }    

}
