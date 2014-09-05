

package com.marvell.ppdgadget;

import java.util.concurrent.BlockingQueue;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class IntentReceiver extends BroadcastReceiver {
    BlockingQueue<Intent> mIntentsQueue;

    public IntentReceiver(BlockingQueue<Intent> queue) {
        super();
        mIntentsQueue = queue;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        //We just enqueue the intent
        mIntentsQueue.offer(intent);
    }
}
