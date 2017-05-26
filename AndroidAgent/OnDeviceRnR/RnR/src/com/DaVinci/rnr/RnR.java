package com.DaVinci.rnr;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class RnR extends Activity {
    private static final int    GET_ALL_APPS              = 1;
    private static final int    CONNECTION_ESTABLISHED    = 2;
    private static final int    CLIENT_GET_READY          = 3;
    private static final int    SCRIPT_RECORDING_STOPPTED = 4;
    private static final int    SCRIPT_UPLOADED           = 5;
    private static long         total                     = 0;
    private static long         accum                     = 0;
    private static final String LOGTAG                    = "DeviceRnR";
    private static final int    MAX_SND_BUFFER_SIZE       = 65535;

    private static final int    STATUS_RECORDING          = 1;
    private static final int    STATUS_STOP               = 2;
    private static final int    STATUS_DELAY              = 3;
    private int                 currentRecordingStatus;

    private Application         currentApp;

    private Socket            channelUSBWiFiSock;
    private LinearLayout      progress;
    private View              manager;
    ProgressDialog            bar;
    private Handler           show;
    private Provider          provider;
    private Adapter           adapter;
    private LayoutInflater    inflater;
    private List<Application> list;
    private List<String>      configuredPackageNames;
    private int               nextPackageRecordIdx;

    private PrintWriter       channelDataWriter;
    private BufferedReader    channelDataReader;
    private DataOutputStream  channelDataOutputBuffer;
    private static int        state                     = 0;
    private ServerSocket      selfServerSock            = null;
    private AlertDialog       adbInfoDialog;

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler() {
        @Override
        public void handleMessage(final Message msg) {
            final String dir = (String) msg.obj;
            switch (msg.what) {
            case GET_ALL_APPS:
                if (state == 0) {
                    // waiting for client gets ready
                    state = 1;
                } else {
                    progress.setVisibility(View.INVISIBLE);
                    adapter = new Adapter(RnR.this);
                    ((GridView) manager.findViewById(R.id.grid)).setAdapter(adapter);
                }

                Log.i(LOGTAG, "Get all APPs");

                LoadConfiguredPackageNames();
                RecordNextConfiguredApp();

                break;
            case CLIENT_GET_READY:
                if (state == 0) {
                    // waiting for all apps information
                    state = 1;
                } else {
                    progress.setVisibility(View.INVISIBLE);
                    adapter = new Adapter(RnR.this);
                    ((GridView) manager.findViewById(R.id.grid)).setAdapter(adapter);
                }

                Log.i(LOGTAG, "Client get ready");
                break;
            case CONNECTION_ESTABLISHED:
                try {
                    // Send CPU arch, SDK version to DaVinci , DaVinci will adb push relative camera_less to the device
                    String arch = Build.CPU_ABI;
                    channelDataWriter.println(arch + String.format(":%d", android.os.Build.VERSION.SDK_INT));
                    Log.i(LOGTAG, String.format("device:%s, %s", arch, android.os.Build.VERSION.SDK_INT));
                } catch (Exception e) {
                    e.printStackTrace();
                }

                new Thread(
                        new Runnable() {
                            @Override
                            public void run() {
                                // Waiting for confirm message from DaVinci after upload camera_less successfully
                                WaitMessage(CLIENT_GET_READY, "ready");
                            }
                        }
                ).start();

                // All apps information and DaVinci are ready, show all apps on main view
                Load();

                Log.i(LOGTAG, "Recording has established");
                break;
            case SCRIPT_RECORDING_STOPPTED:
                Log.i(LOGTAG, "Recording has stopped");
                currentRecordingStatus = STATUS_STOP;
                bar.show();
                bar.setMessage("uploading scripts for app " + (String) msg.obj);
                new Thread(
                        new Runnable() {
                            @Override
                            public void run() {
                                WaitMessage(SCRIPT_UPLOADED, "uploaded");
                            }
                        }
                        ).start();
                break;
            case SCRIPT_UPLOADED:
                // All files have been transferred to DaVinci successfully
                Log.i(LOGTAG, String.format("Upload finished for ", (String) msg.obj));
                bar.dismiss();
                RecordNextConfiguredApp();
                break;
            default:
                break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        currentRecordingStatus = STATUS_DELAY;

        inflater = getLayoutInflater();
        manager = inflater.inflate(R.layout.rnr, (ViewGroup) findViewById(R.id.manager));
        bar = new ProgressDialog(manager.getContext());
        show = new Handler();
        bar.setMessage("script uploading:");
        bar.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        bar.setCancelable(false);
        bar.setProgress(0);
        bar.setMax(100);

        ShowADBInfoDialog();

        new Thread() {
            @Override
            public void run() {
                    try {
                        // listen on port 9999 for tcp connection from DaVinci over USB cable
                        selfServerSock = new ServerSocket(9999);

                        while (!Thread.currentThread().isInterrupted()) {
                            channelUSBWiFiSock = selfServerSock.accept();
                            channelUSBWiFiSock.setSendBufferSize(MAX_SND_BUFFER_SIZE);

                            DismissADBInfoDialog();

                            // prepare for data transmission
                            channelDataWriter = new PrintWriter(new BufferedWriter(new OutputStreamWriter(channelUSBWiFiSock.getOutputStream())), true);
                            channelDataReader = new BufferedReader(new InputStreamReader(channelUSBWiFiSock.getInputStream()));
                            channelDataOutputBuffer = new DataOutputStream(channelUSBWiFiSock.getOutputStream());

                            // the tcp connection will be used in the same way as to over WIFI
                            Message msg = new Message();
                            msg.what = CONNECTION_ESTABLISHED;
                            handler.sendMessage(msg);
                        }
                    } catch (Exception e) {
                        Log.e(LOGTAG, e.toString());
                        e.printStackTrace();
                    }
                }
            }.start();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        // TODO Auto-generated method stub
        super.onConfigurationChanged(newConfig);
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            // Nothing need to be done here
            Log.i(LOGTAG, "On Config Change : LANDSCAPE");
        } else {
            // Nothing need to be done here
            Log.i(LOGTAG, "On Config Change : PORTRAIT");
        }
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        if (currentRecordingStatus == STATUS_RECORDING) {
            channelDataWriter.println();
        }
    }

    /* show the main view of launcher */
    protected void Load() {
        setContentView(manager);

        ((GridView) manager.findViewById(R.id.grid)).setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                currentApp = list.get(position);
                currentRecordingStatus = STATUS_RECORDING;
                Start(currentApp);
            }
        });

        progress = (LinearLayout) findViewById(R.id.progress);
        progress.setVisibility(View.VISIBLE);

        /* get all launchable apps information to construct the launcher UI */
        new Thread() {
            @Override
            public void run() {
                provider = new Provider(RnR.this);
                list = provider.getAllApps();
                Message msg = new Message();
                msg.what = GET_ALL_APPS;
                handler.sendMessage(msg);
            };
        }.start();
    }

    /* wait for 'message' from DaVici, and send 'event' to handler */
    protected void WaitMessage(final int event, String message) {
        try {
            String readBuffer = "";
            do {
                readBuffer = channelDataReader.readLine();
                if (readBuffer == null) {
                    Thread.sleep(100);
                    continue;
                }
                if (readBuffer.startsWith("bar:"))
                {
                    Indicate(Integer.parseInt(readBuffer.substring(4)));
                }
            } while (readBuffer.equals(message) == false);

            Message msg = new Message();
            msg.what = event;
            msg.obj = message;
            handler.sendMessage(msg);
        } catch (IOException e) {
            Log.e(LOGTAG, e.toString());
            e.printStackTrace();
        } catch (InterruptedException e) {
            Log.e(LOGTAG, e.toString());
            e.printStackTrace();
        } catch (Exception e) {
            Log.e(LOGTAG, e.toString());
            e.printStackTrace();
        }
    }

    protected void ShowADBInfoDialog() {
        AlertDialog.Builder adbInfoDialogBuilder = new AlertDialog.Builder(RnR.this);
        adbInfoDialogBuilder.setIcon(R.drawable.ic_launcher);
        adbInfoDialogBuilder.setTitle("Prerequisite");
        adbInfoDialogBuilder.setMessage("1. Connect to host via ADB " + "\n" + "2. Wait DaVinci attaching");
        adbInfoDialogBuilder.setCancelable(false);
        adbInfoDialogBuilder.setNegativeButton("Cannel", new TargetServerCancelListener());
        adbInfoDialog = adbInfoDialogBuilder.create();
        adbInfoDialog.show();
    }

    protected void DismissADBInfoDialog() {
        if (adbInfoDialog != null) {
            adbInfoDialog.cancel();
            adbInfoDialog.dismiss();
        }
    }

    /* start recording, show a dialog to stop it */
    protected void Start(final Application info) {
        try {
            Log.i(LOGTAG, "Start APP. "
                          + info.getPackageName()
                          + ":" + info.getPackageName()
                          + ":" + info.getActivityName());

            // store all script files in the folder named after the app
            channelDataWriter.println(info.getPackageName()
                                      + ":" + info.getPackageName()
                                      + ":" + info.getActivityName()
                                      );
            // launch the app
            Intent intent = getPackageManager().getLaunchIntentForPackage(info.getPackageName());
            if (intent != null) {
                startActivity(intent);
                new Thread() {
                    @Override
                    public void run() {
                        // waiting for message sent from DaVinci indicating
                        // recording has stopped
                        WaitMessage(SCRIPT_RECORDING_STOPPTED, info.getPackageName());
                    };
                }.start();
            }
        } catch (Exception e) {
            Log.e(LOGTAG, e.toString());
            e.printStackTrace();
        }
    }

    // show progress script transmission
    protected void Indicate(final int percentage) {
        show.post(new Runnable() {
            @Override
            public void run() {
                bar.setProgress(percentage);
            }
        });
    }

    protected void LoadConfiguredPackageNames() {
        nextPackageRecordIdx = 0;
        configuredPackageNames = new ArrayList<String>();
        File config = new File("/data/local/tmp/D9D8B662-577D-4F52-BE3A-AAB4508D51DE");
        if (!config.exists())
        {
            Log.i(LOGTAG, "Configured Package Name file doesn't exist");
            return;
        }

        try {
            BufferedReader br = null;
            br = new BufferedReader(new FileReader(config));
            String line = null;
            while ((line = br.readLine()) != null) {
                configuredPackageNames.add(line);
            }
            br.close();
        } catch (FileNotFoundException e) {
            configuredPackageNames.clear();
            Log.e(LOGTAG, e.toString());
            e.printStackTrace();
        } catch (IOException e) {
            configuredPackageNames.clear();
            Log.e(LOGTAG, e.toString());
            e.printStackTrace();
        }
    }

    protected void RecordNextConfiguredApp() {
        if (nextPackageRecordIdx >= 0 &&
            nextPackageRecordIdx < configuredPackageNames.size()) {
            for (int i = 0; i < list.size(); i++) {
                Application app = list.get(i);
                String packageNameConfig = configuredPackageNames.get(nextPackageRecordIdx);
                String[] packageNameConfigSplit = packageNameConfig.split(":");
                if (packageNameConfigSplit[0].equals(app.getPackageName())) {
                    if (packageNameConfigSplit.length > 1) {
                        try {
                            // copy the configured trace file to the GUID file
                            File source = new File(packageNameConfigSplit[1]);
                            File dest = new File("/data/local/tmp/A0A2338C-7CBE-42E3-AF43-4D0BB82E9968");
                            InputStream in = new FileInputStream(source);
                            OutputStream out = new FileOutputStream(dest);
                            byte[] buf = new byte[1024];
                            int len;
                            while ((len = in.read(buf)) > 0) {
                                out.write(buf, 0, len);
                            }
                            in.close();
                            out.close();
                        }
                        catch (IOException e) {
                            Log.e(LOGTAG, e.toString());
                            e.printStackTrace();
                        }
                    }
                    currentRecordingStatus = STATUS_RECORDING;
                    Start(app);
                    break;
                }
            }
            nextPackageRecordIdx++;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // The activity is about to be destroyed.

        try {
            if (channelDataWriter != null) {
                channelDataWriter.println();
            }

            if (channelUSBWiFiSock != null) {
                channelUSBWiFiSock.close();
            }

            if (selfServerSock != null) {
                selfServerSock.close();
            }
        } catch (Exception e) {
            Log.e(LOGTAG, e.toString());
            e.printStackTrace();
        }
    }

    // Adapter for cell in gridview of Launcher
    private class Adapter extends BaseAdapter {
        private RnR context;

        public Adapter(RnR context) {
            this.context = context;
        }

        @Override
        public int getCount() {
            return list.size();
        }

        @Override
        public Object getItem(int position) {
            return list.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            Application info = list.get(position);

            if (convertView == null) {
                convertView = LayoutInflater.from(this.context).inflate(R.layout.item, null);
            }
            ImageView icon = (ImageView) convertView.findViewById(R.id.icon);
            TextView name = (TextView) convertView.findViewById(R.id.name);
            icon.setImageDrawable(info.getIcon());
            name.setText(info.getAppName());
            return convertView;
        }
    }

    private class TargetServerCancelListener implements DialogInterface.OnClickListener {
        @Override
        public void onClick(DialogInterface dialog, int id) {
            finish();
            System.exit(0);
        }
    }

}
