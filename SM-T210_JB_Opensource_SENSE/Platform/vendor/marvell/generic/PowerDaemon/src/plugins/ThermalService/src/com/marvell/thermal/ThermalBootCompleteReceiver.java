package com.marvell.thermal;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class ThermalBootCompleteReceiver extends BroadcastReceiver {
	public static String TAG = "ThermalBootCompleteReceiver";

	@Override
	public void onReceive(Context context, Intent intent) {
		// start thermal service after android boot complete
		if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
			Log.i(TAG, "start thermal service");
			Intent sIntent = new Intent();
			sIntent.setClassName("com.marvell.thermal",
					"com.marvell.thermal.ThermalService");
			context.startService(sIntent);
		}
	}
}