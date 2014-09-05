package com.marvell.thermal;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.Notification.Builder;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;

public class ThermalService extends Service {
    public static String TAG = "ThermalService";

    //values of thermal trip point, must match to the kernel setting
    private static int NOTICEABLE_TRIP_POINT = 85000;
    private static int WARNING_TRIP_POINT = 95000;
    private static int CRITICAL_TRIP_POINT = 105000;

    //values of thermal state
    private static final String STATE_SAFE = "safe";
    private static final String STATE_NOTICEABLE = "noticeable";
    private static final String STATE_WARNING = "warning";
    private static final String STATE_CRITICAL = "critical";

    public static final String THERMAL_STATE_CHANGED_ACTION = "com.marvell.thermal.STATE_CHANGED";

    private Context mContext;
    private NotificationManager mNM = null;
    private ThermalUEventObserver mUEventObserver = null;
    private static final int THERMAL_NOTIFICATION_ID = 1;
    private String mThermalState = "safe";

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG,"Run Thermal Service!");
        mNM = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

        mUEventObserver = new ThermalUEventObserver(this);
        mUEventObserver.startObserving("SUBSYSTEM=thermal");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        mUEventObserver.stopObserving();
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    String getThermalState() {
        return mThermalState;
    }

    void setThermalState(int temp) {
        if (temp >= CRITICAL_TRIP_POINT) {
            mThermalState = STATE_CRITICAL;
            shutdown();
        } else if (temp >= WARNING_TRIP_POINT) {
            mThermalState = STATE_WARNING;
            warning();
        } else if (temp >= NOTICEABLE_TRIP_POINT) {
            mThermalState = STATE_NOTICEABLE;
        } else {
            mThermalState = STATE_SAFE;
            dismissWarning();
        }

        //send thermal state intent
        Intent mIntent = new Intent(THERMAL_STATE_CHANGED_ACTION);
        mIntent.putExtra("state", mThermalState);
        sendBroadcast(mIntent);
    }

    private void warning() {
        Builder nb = new Notification.Builder(this);
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0, new Intent(
                Settings.ACTION_APPLICATION_SETTINGS), 0);
        nb.setContentIntent(contentIntent);
        nb.setAutoCancel(true);
        nb.setContentTitle("System is overheated");
        nb.setContentText("Select to release system resources");
        nb.setTicker("System is overheated");
        nb.setSmallIcon(R.drawable.thermal_too_hot);
        mNM.notify(THERMAL_NOTIFICATION_ID, nb.getNotification());
    }

    private void dismissWarning() {
        mNM.cancelAll();
    }

    private void shutdown() {
        Log.d(TAG, "Shutdown for overheated...");
        Intent shutdown = new Intent(Intent.ACTION_REQUEST_SHUTDOWN);
        shutdown.putExtra(Intent.EXTRA_KEY_CONFIRM, false);
        shutdown.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(shutdown);
    }
}
