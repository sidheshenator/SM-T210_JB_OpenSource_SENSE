package com.marvell.thermal;

import android.os.UEventObserver;
import android.util.Log;

public class ThermalUEventObserver extends UEventObserver {
    public static String TAG = "ThermalUEventObserver";
    private ThermalService service = null;

    public ThermalUEventObserver(ThermalService thermalService) {
        service = thermalService;
    }

    @Override
    public void onUEvent(UEventObserver.UEvent event) {
        Log.d(TAG, "Received thermal uevent: " + event.toString());
        update(event.get("DEVPATH"), Integer.parseInt(event.get("TEMP")));
    }

    public void update(String dev, int temp) {
        service.setThermalState(temp);
    }
}
