package com.marvell.ddrhotplug;

import android.os.UEventObserver;
import android.util.Log;

public class DDRHotplugUEventObserver extends UEventObserver {
    public static String TAG = "DDRHotplugUEventObserver";
    private DDRHotplugService service = null;

    public DDRHotplugUEventObserver(DDRHotplugService ddrHotplugService) {
        service = ddrHotplugService;
    }

    @Override
    public void onUEvent(UEventObserver.UEvent event) {
        Log.d(TAG, "Received ddrhotplug uevent: " + event.toString());
        //update(event.get("DEVPATH"), Integer.parseInt(event.get("TEMP")));
    }

    public void update(String dev, boolean state) {
        service.setHotplugState(state);
    }
}
