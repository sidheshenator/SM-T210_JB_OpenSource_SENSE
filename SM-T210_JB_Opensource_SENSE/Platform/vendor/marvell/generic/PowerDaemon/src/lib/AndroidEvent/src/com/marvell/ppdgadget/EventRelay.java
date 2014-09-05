

package com.marvell.ppdgadget;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.OutputStream;
import java.io.File;

import java.lang.InterruptedException;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserFactory;
import org.xmlpull.v1.XmlPullParserException;


import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;

public class EventRelay extends Service {
    static final String TAG = "EventRelay";

    private final String LEGACY_CONFIG_FILE_PATH = "/etc/powerdaemon.xml";
    private final String CONFIG_FILE_PATH = "/etc/powerdaemon_z3.xml";
    private final int Z3_REVISION = 2;

    private final String DVFS_BOOSTER_ACTION = "com.sec.android.intent.action.MRVL_DVFS_BOOSTER";
    private final int MSG_BOOST_END = 0;
    private final int MSG_BOOST_START = 1;
    private final int BOOSTER_ENABLE_MASK = 0xFFFFFFFF;
    private int mBoosterStatus;

    //FIXME: maybe more status are needed for booster
    private final int BOOSTER_ERROR = -1;           //ERROR STATUS
    private final int BOOSTER_SSG = 0;              //SSG DVFS BOOSTER
    private final int BOOSTER_WEBKIT = 1;           //BROWSER
    private final int BOOSTER_WEBKIT_BENCH = 2;     //BROWSER BENCHMARK
    private final int BOOSTER_ROTATION = 3;         //ROTATION

    //BOOST REASON ARRAY, index and string must match above definition
    private final String[] mBoostReason = new String[]{"","BROWSER", "BROWSER_EXTREME", "ROTATION"};

    private long mDvfsBoosterCancelTime = -1; // cancel time for booster
    private Object mCancelLockObj = new Object(); // lock for mDvfsBoosterCancelTime
    private final boolean DEBUG_BOOSTER = true;

    static {
        System.loadLibrary("AndroidEvent");
    }

    private int mSocketSrvFd; //fd of socket server
    private BroadcastReceiver mReceiver;
    private BlockingQueue<Intent> mPendingIntents = new LinkedBlockingQueue<Intent>();

    private IntentFilter parseConfiguration()
            throws XmlPullParserException, FileNotFoundException, IOException {
        IntentFilter filter = new IntentFilter();

        XmlPullParser parser = XmlPullParserFactory.newInstance().newPullParser();

        // judge the chip revision
        int chipRev = getChipRevison();
        File configFile = null;
        if (chipRev >= Z3_REVISION) {
            configFile = new File(CONFIG_FILE_PATH);
        } else {
            configFile = new File(LEGACY_CONFIG_FILE_PATH);
        }

        BufferedReader br = new BufferedReader(new FileReader(configFile));
        parser.setInput(br);

        int eventType = parser.getEventType();
        while (eventType != XmlPullParser.END_DOCUMENT) {
            switch (eventType) {
                case XmlPullParser.START_TAG:
                    String tag = parser.getName();
                    if (tag.equals("event")) { //handle the event tag
                        String intentName = parser.getAttributeValue(null,"intent");
                        if (intentName == null) {
                            Log.e(TAG,"No intent attribute! Illegal Config Format?");
                            return filter;
                         } else if (intentName.equals(Intent.ACTION_BOOT_COMPLETED)){
                            //If we are interested in BOOT_COMPLETED, queue it.
                            mPendingIntents.offer(new Intent(Intent.ACTION_BOOT_COMPLETED));
                        } else {
                            Log.d(TAG,"add Action: " + intentName);
                            filter.addAction(intentName);
                        }
                    }
                    break;
                default:
                    break;
            }
            eventType = parser.next();
        }
        br.close();

        //for DVFS_BOOSTER
        filter.addAction(DVFS_BOOSTER_ACTION);
        Log.d(TAG,"add Action: " + DVFS_BOOSTER_ACTION);
        return filter;
    }

    public void onCreate() {
        mBoosterStatus = 0;

        IntentFilter filter;
        mReceiver = new IntentReceiver(mPendingIntents);

        //parse the configuration of PowerDaemon
        try {
            filter = parseConfiguration();
        } catch (IOException ex) {
            Log.d(TAG, "onCreate: read configuration: " + ex);
            return;
        } catch (XmlPullParserException ex){
            Log.d(TAG, "onCreate: parse configuration: " + ex);
            return;
        }

        //register receiver
        registerReceiver(mReceiver, filter);

        Log.d(TAG,"OnCreate: Open socket!");

        //open the connection
        mSocketSrvFd = openConnectionToSocketSrv();
    }

    //Write the cmd to the socket server with Intent format
    private native void sendCmdToSocketSrv(Intent i, int socketFd);
    //Write the cmd to the socket server with specific format
    private native void sendStateCmdToSocketSrv(int state, int socketFd);
    //Open the connection of socket server
    private native int openConnectionToSocketSrv();
    //Close the connection of socket server
    private native int closeConnectionToSocketSrv(int socketFd);
    //Get chip revision
    private native int getChipRevison();

    // The cancel tokens for each Booster message.
    private BoosterCancelMsgInfo mBoosterSSGCancelMsg = new BoosterCancelMsgInfo(new Object(), -1);
    private BoosterCancelMsgInfo mBoosterWebKitCancelMsg = new BoosterCancelMsgInfo(new Object(), -1);
    private BoosterCancelMsgInfo mBoosterWebKitBenchCancelMsg = new BoosterCancelMsgInfo(new Object(), -1);
    private BoosterCancelMsgInfo mBoosterRotationCancelMsg = new BoosterCancelMsgInfo(new Object(), -1);

    private class BoosterCancelMsgInfo {
        public Object token;
        public long cancelTime;

        BoosterCancelMsgInfo(Object token, long cancelTime) {
            this.token = token;
            this.cancelTime = cancelTime;
        }
    }

    // Components for DVFS_BOOSTER
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            int reason = msg.arg1;
            int tag = 1 << reason;
            switch (msg.what) {
                case MSG_BOOST_END:
                    tag = ~tag;
                    mBoosterStatus &= tag;
                    break;
                case MSG_BOOST_START:
                    mBoosterStatus |= tag;
                    break;
                default:
                    Log.i(TAG, "Error status for Booster: " + msg);
            }
            if (DEBUG_BOOSTER) {
                Log.i(TAG, "sendStateCmdToSocketSrv--reason: " + reason
                    + " status: " + Integer.toBinaryString(mBoosterStatus));
            }
            sendStateCmdToSocketSrv(mBoosterStatus, mSocketSrvFd);
        }
    };

    private int getReasonFromIntent(Intent i) {
        String reason = i.getStringExtra("BOOST_REASON");
        int status = BOOSTER_ERROR;
        if (reason == null) {
            return BOOSTER_SSG;
        } else {
            for(int idx = 0; idx < mBoostReason.length; idx++) {
                if (reason.equals(mBoostReason[idx]))
                    return idx;
            }
        }
        Log.e(TAG, "invalid boost reason value: " + reason);
        return status;
    }

    private BoosterCancelMsgInfo getMsgInfoForReason(int reason) {
        BoosterCancelMsgInfo info = null;
        switch (reason) {
            case BOOSTER_SSG:
                info = mBoosterSSGCancelMsg;
                break;
            case BOOSTER_WEBKIT:
                info = mBoosterWebKitCancelMsg;
                break;
            case BOOSTER_WEBKIT_BENCH:
                info = mBoosterWebKitBenchCancelMsg;
                break;
            case BOOSTER_ROTATION:
                info = mBoosterRotationCancelMsg;
                break;
            default:
                Log.e(TAG, "invalid boost reason value: " + reason);
        }
        return info;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        //Spawn the bridger
        Thread bridger = new Thread(){
            @Override
            public void run() {
                Intent i = null;
                while (true) {
                    try{
                        i = mPendingIntents.take();
                    } catch (Exception ex) {
                        Log.d(TAG, "onStartCommand: get intent failed!" + ex);
                        return;
                    }

                    if (i.getAction().equals(DVFS_BOOSTER_ACTION)) {
                        //DVFS_BOOSTER
                        // Currently, DVFS_BOOSTER has two args:
                        // DURATION:    The duration time for the boost requirement.
                        //      positive number: normal case, indicates the duration for the requirment
                        //      0:Forever requirement, it would be canceled until received cancel cmd.
                        //      -1: cancel cmd, would cancel the requirement immediately
                        //
                        // REASON:      The booster reason, different policy for differnet reason.
                        //      null: stands for SSG default DVFS_BOOSTER
                        //      WEBKIT: for broswer case
                        //      ROTATION: for screen rotation case
                        //
                        String duration_ = i.getStringExtra("DURATION");
                        int duration = -1;
                        if (null != duration_) {
                            duration = Integer.parseInt(duration_);
                        } else {
                            Log.e(TAG, "NULL duration time value!");
                            continue;
                        }

                        int reason = getReasonFromIntent(i);

                        if (DEBUG_BOOSTER) {
                            Log.d(TAG, "Got DVFS_BOOSTER: reason " +
                                i.getStringExtra("BOOST_REASON") + ":" + reason + " duration: " + duration);
                        }

                        //invalid boost reason
                        if (reason < 0)
                            continue;

                        // Explicit cancel cmd
                        if (duration == -1) {
                            if (DEBUG_BOOSTER) Log.d(TAG, "Got explicit cancel cmd for: " + reason);
                            BoosterCancelMsgInfo info = getMsgInfoForReason(reason);
                            Message endMsg = mHandler.obtainMessage(MSG_BOOST_END, reason, duration, info.token);
                            mHandler.removeMessages(MSG_BOOST_END, info.token);
                            mHandler.sendMessage(endMsg);
                            continue;
                        }

                        Message startMsg = mHandler.obtainMessage(MSG_BOOST_START, reason, duration);
                        mHandler.sendMessage(startMsg);

                        if (duration != 0) { // duration ==0, means we should last this policy forever, no cancel msg
                            BoosterCancelMsgInfo info = getMsgInfoForReason(reason);
                            long cancelTime = SystemClock.uptimeMillis() + duration;
                            Message endMsg = mHandler.obtainMessage(MSG_BOOST_END, reason, duration, info.token);
                            if (mHandler.hasMessages(MSG_BOOST_END, info.token)) {
                                // Currently, we have cancel msg in the queue. For cancel time,
                                // if new one > old one, cancel the old one, set new one
                                // if new one < old one, ignore current one
                                if (cancelTime > info.cancelTime) {
                                    // Cancel the old one, update info and send a new msg
                                    if (DEBUG_BOOSTER) Log.d(TAG, "Cancel old cancel message for: " + reason);
                                    mHandler.removeMessages(MSG_BOOST_END, info.token);
                                    info.cancelTime = cancelTime;
                                    mHandler.sendMessageAtTime(endMsg, cancelTime);
                                } else {
                                    if (DEBUG_BOOSTER) Log.d(TAG, "Ignore current cancel message for: " + reason);
                                }
                            } else {
                                // No cancel msg, queue one
                                info.cancelTime = SystemClock.uptimeMillis() + duration;//update cancel time
                                if (DEBUG_BOOSTER)
                                    Log.d(TAG, "queue new cancel msg for: " + reason + " at " + info.cancelTime);

                                mHandler.sendMessageDelayed(endMsg, duration);
                            }
                        }
                    } else {
                        //general case
                        Log.i(TAG, "Send intent to cmd socket: " + i);
                        sendCmdToSocketSrv(i, mSocketSrvFd);
                    }
                }
            }
        };
        bridger.start();
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy: Maybe something happened! ");
        closeConnectionToSocketSrv(mSocketSrvFd);
        unregisterReceiver(mReceiver);
    }
}
