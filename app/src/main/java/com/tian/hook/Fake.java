package com.tian.hook;

import android.content.pm.PackageInfo;

/**
 * Created by tian.ke on 2018/2/13.
 */

public class Fake {
    public native PackageInfo getPackageInfo(String packageName, int flags);
}
