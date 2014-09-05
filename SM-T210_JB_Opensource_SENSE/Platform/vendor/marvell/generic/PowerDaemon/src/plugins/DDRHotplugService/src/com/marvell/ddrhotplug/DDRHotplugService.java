package com.marvell.ddrhotplug;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.ActivityManager;
import android.app.Notification.Builder;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.IBinder;
import android.os.SystemProperties;
import android.os.Process;
import android.os.Debug;
import android.os.UEventObserver;
import android.provider.Settings;
import android.util.Log;

import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;

public class DDRHotplugService extends Service {
    public static String TAG = "DDRHotplugService";
    public static final String DDR_HOTPLUG_ONLINE_ACTION = "com.marvell.ddrhotplug.ONLINE";
    public static final String DDR_HOTPLUG_OFFLINE_ACTION = "com.marvell.ddrhotplug.OFFLINE";

    private DDRHotplugUEventObserver mUEventObserver = null;
    private boolean mDDROfflineState = false;
    private IntentFilter mScreenStateFilter;
    private final boolean DEBUG_MEM_HOTPLUG = true;

    static {
        System.loadLibrary("ddr_hotplug");
    }

    private native int dumpMemInfo();

    private final BroadcastReceiver mScreenStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
                setHotplugState(true);
            } else if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF)) {
                setHotplugState(false);
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG,"Run DDR Hotplug Service!");

        mUEventObserver = new DDRHotplugUEventObserver(this);
        //mUEventObserver.startObserving("SUBSYSTEM=thermal");

        mScreenStateFilter = new IntentFilter();
        mScreenStateFilter.addAction(Intent.ACTION_SCREEN_OFF);
        mScreenStateFilter.addAction(Intent.ACTION_SCREEN_ON);

        registerReceiver(mScreenStateReceiver, mScreenStateFilter);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        mUEventObserver.stopObserving();
        unregisterReceiver(mScreenStateReceiver);
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    boolean getHotplugState() {
        return mDDROfflineState;
    }

    void setHotplugState(boolean state) {
        mDDROfflineState = state;
        Intent i = null;

        if (state) { // online
            i = new Intent(DDR_HOTPLUG_ONLINE_ACTION);
        } else { // offline
            doMemHotplugCleanup();
            i = new Intent(DDR_HOTPLUG_OFFLINE_ACTION);
        }

        //send ddr hotplug state intent
        sendBroadcast(i);
    }

    private String processUeventData(UEventObserver.UEvent event){
        //TO DO: process the uevent data
        return null;
    }

    private boolean checkForMemHotplug(String memState){
        //TO DO: process the uevent data
        return false;
    }

    private List<String> getAllLaunchers() {
        List<String> result = null;

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);

        PackageManager pkgMgt = getPackageManager();
        List<ResolveInfo> allLaunchers = pkgMgt.queryIntentActivities(intent, 0);
        if (allLaunchers.size() != 0) {
            result = new ArrayList<String>();
        } else {
            return result;
        }

        for (int i=0; i<allLaunchers.size(); i++) {
            String packageName = allLaunchers.get(i).activityInfo.packageName;
            result.add(packageName);
        }
        return result;
    }

    private boolean isLauncher(String processName) {
        List<String> allLaunchers = getAllLaunchers();
        if ((null == allLaunchers) || (0 == allLaunchers.size())) {
            return false;
        }

        for (String launcher : allLaunchers) {
            if (processName.equals(launcher)) {
                return true;
            }
        }
        return false;
    }

    private List<ActivityManager.RunningAppProcessInfo> mRunningAppProcessInfo = null;
    private static final int KILLING_THRESHOLD = 2048; // selected apps size > 2M

    /*
     *  Some procs that it should be skipped for be killed.
     */
    private boolean isSkippedProcess(String processName) {
        return processName.equals("android.process.acore")
                    || processName.equals("com.android.phone")
                    || processName.equals("android.process.media")
                    || processName.equals("system");
    }

    /*
     *  Judge whether the proc should be killed or not.
     */
    private boolean isShouldBeKilled(ActivityManager.RunningAppProcessInfo processInfo) {
        return (0 == (ActivityManager.RunningAppProcessInfo.FLAG_PERSISTENT &
                    processInfo.flags))
                    && (processInfo.uid >= Process.FIRST_APPLICATION_UID)
                    && (processInfo.importance != ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND)
                    && (processInfo.importance != ActivityManager.RunningAppProcessInfo.IMPORTANCE_PERCEPTIBLE)
                    && (!isLauncher(processInfo.processName))
                    && (!isSkippedProcess(processInfo.processName));
    }

    /*
     *  Mapping the importance into string
     */
    private String getProcessImportance(int importance) {
        String result = null;
        switch (importance) {
            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_BACKGROUND:
                 result = "IMPORTANCE_BACKGROUND";
                 break;

            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_CANT_SAVE_STATE:
                 result = "IMPORTANCE_CANT_SAVE_STATE";
                 break;

            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_EMPTY:
                 result = "IMPORTANCE_EMPTY";
                 break;

            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND:
                 result = "IMPORTANCE_FOREGROUND";
                 break;

            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_PERCEPTIBLE:
                 result = "IMPORTANCE_PERCEPTIBLE";
                 break;

            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_PERSISTENT:
                 result = "IMPORTANCE_PERSISTENT";
                 break;

            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_SERVICE:
                 result = "IMPORTANCE_SERVICE";
                 break;

            case ActivityManager.RunningAppProcessInfo.IMPORTANCE_VISIBLE:
                 result = "IMPORTANCE_VISIBLE";
                 break;

            default:
                 Log.e(TAG, "Illegal importance for processInfo");
        }
        return result;
    }

    private void dumpAllRunningProcInfo(List<ActivityManager.RunningAppProcessInfo> appInfos){
        Log.d(TAG, "**************************");
        for (ActivityManager.RunningAppProcessInfo appInfo : appInfos) {
            Log.d(TAG, "Process: " + appInfo.processName
                + " uid: " + appInfo.uid + " pid: " + appInfo.pid);
        }
        Log.d(TAG, "**************************");
    }

    /*
     * Do the memory hotplug clean up.
     */
    private void doMemHotplugCleanup(){
        // Get all the process state first, uid > 1000
        // FIXME: right now just running app state
        int i,freeBeforeKill,freeAfterKill,pss;
        int[] pids;
        final ActivityManager am = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
        mRunningAppProcessInfo = am.getRunningAppProcesses();

        if (mRunningAppProcessInfo == null) {
            Log.e(TAG, "Get null when try to getRunningAppProcesses");
        } else if (mRunningAppProcessInfo.size() > 0) {
            int size = mRunningAppProcessInfo.size();
            int pidNum = 0;
            int[] allRunningPids = new int[size];
            HashMap<Integer,String> procToKilledInfo = new HashMap<Integer,String>();

            if (DEBUG_MEM_HOTPLUG)
                dumpAllRunningProcInfo(mRunningAppProcessInfo);

            for (ActivityManager.RunningAppProcessInfo processInfo: mRunningAppProcessInfo) {
                if (isShouldBeKilled(processInfo)) {
                    allRunningPids[pidNum] = processInfo.pid;
                    pidNum++;
                    procToKilledInfo.put(new Integer(processInfo.pid), processInfo.processName);
                }
            }

            //filter the pids
            pids = new int[pidNum];
            for (i = 0; i < pidNum; i++) {
                pids[i] = allRunningPids[i];
            }

            if (DEBUG_MEM_HOTPLUG) {
                Log.e(TAG, "==========================");
                for (int pid : pids) {
                    Log.d(TAG, "Process to be killed!!!: " + pid);
                }

                freeBeforeKill = dumpMemInfo();
                Log.d(TAG, "==========================");
            }

            Debug.MemoryInfo[] memInfos =  am.getProcessMemoryInfo(pids);
            for (i = 0; i < memInfos.length; i++) {
                Debug.MemoryInfo mi = memInfos[i];
                pss = mi.getTotalPss();
                /* kill all the process lager than KILLING_THRESHOLD */
                if (pss > KILLING_THRESHOLD) {
                    if (DEBUG_MEM_HOTPLUG) {
                        Log.d(TAG, "Kill " + pids[i] + " name: "
                            + procToKilledInfo.get(new Integer(pids[i])) + " PSS: " + pss);
                    }
                    am.forceStopPackage(procToKilledInfo.get(new Integer(pids[i])));
                }
            }

            if (DEBUG_MEM_HOTPLUG) {
                freeAfterKill = dumpMemInfo();
                Log.d(TAG, "All kill done, saving " + (freeAfterKill - freeBeforeKill) + " KB!");
                Log.d(TAG, "==========================");
            }
        }
    }


}
