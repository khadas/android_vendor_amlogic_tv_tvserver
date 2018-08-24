package com.droidlogic.app.tv;

import android.content.Context;
import android.provider.Settings;
import android.os.SystemClock;
import android.util.Log;

import java.util.Date;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.DaylightSavingTime;
import com.droidlogic.app.tv.TvControlDataManager;

public class TvTime{
    private long diff = 0;
    private Context mContext;

    private final static String TV_KEY_TVTIME = "dtvtime";
    private final static String PROP_SET_SYSTIME_ENABLED = "persist.sys.getdtvtime.isneed";
    private TvControlDataManager mTvControlDataManager = null;

    public TvTime(Context context){
        mContext = context;
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);
    }

    public synchronized void setTime(long time){
        Date sys = new Date();

        diff = time - sys.getTime();
        SystemControlManager SM = new SystemControlManager(mContext);
        if (SM.getPropertyBoolean(PROP_SET_SYSTIME_ENABLED, false)
                && (Math.abs(diff) > 1000)) {
            SystemClock.setCurrentTimeMillis(time);
            diff = 0;
            DaylightSavingTime daylightSavingTime = DaylightSavingTime.getInstance();
            daylightSavingTime.updateDaylightSavingTimeForce();
            Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.AUTO_TIME, 0);
            Log.d("DroidLogic", "setTime");
        }

        mTvControlDataManager.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, diff);
    }


    public synchronized long getTime(){
        Date sys = new Date();
        diff = mTvControlDataManager.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);

        return sys.getTime() + diff;
    }


    public synchronized long getDiffTime(){
        return mTvControlDataManager.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);
    }

    public synchronized void setDiffTime(long diff){
        this.diff = diff;
        mTvControlDataManager.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, this.diff);
    }
}
