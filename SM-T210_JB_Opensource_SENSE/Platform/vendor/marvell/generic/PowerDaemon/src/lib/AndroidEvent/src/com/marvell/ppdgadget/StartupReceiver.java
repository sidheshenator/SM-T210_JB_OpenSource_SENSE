

package com.marvell.ppdgadget;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class StartupReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        Intent sIntent = new Intent();
        sIntent.setAction("com.marvell.ppdgadget.EventRelay");
        context.startService(sIntent);
    }
}
