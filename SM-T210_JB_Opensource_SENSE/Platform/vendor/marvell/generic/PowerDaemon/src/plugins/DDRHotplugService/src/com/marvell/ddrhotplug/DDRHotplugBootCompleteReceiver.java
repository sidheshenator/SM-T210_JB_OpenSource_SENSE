package com.marvell.ddrhotplug;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class DDRHotplugBootCompleteReceiver extends BroadcastReceiver {
    public static String TAG = "DDRHotplugBootCompleteReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        // start thermal service after android boot complete
        if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
            Log.i(TAG, "start DDR Hotplug service");
            Intent sIntent = new Intent();
            sIntent.setClassName("com.marvell.ddrhotplug",
                    "com.marvell.ddrhotplug.DDRHotplugService");
            context.startService(sIntent);
        }
    }
}
