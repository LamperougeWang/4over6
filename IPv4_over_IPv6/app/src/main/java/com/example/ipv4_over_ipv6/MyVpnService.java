package com.example.ipv4_over_ipv6;

import android.app.PendingIntent;
import android.content.Intent;
import android.net.VpnService;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.StrictMode;
import android.util.Log;
import android.widget.Toast;

import java.io.IOException;
import java.net.InetSocketAddress;

/**
 * Created by alexzhangch on 2017/4/13.
 */

public class MyVpnService extends VpnService implements Handler.Callback, Runnable{

    private static final String TAG = "Top Vpn";

    // VPN 服务器ip地址
    private String mServerAddress = "2402:f000:1:4417::900";
    private int mServerPort = 5678;


    // 虚接口数据, 通过100请求获得
    String ipv4_Addr;
    String router;
    String DNS1;
    String DNS2;
    String DNS3;

    Builder builder = new Builder();

    private PendingIntent mConfigureIntent;
    // 用于输出调试信息 （Toast）
    private Handler mHandler;
    private ParcelFileDescriptor mInterface;

    private Thread mThread;


    // Used to load the 'vpn_service' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public int responseIPv4(String ipv4) {
        Log.d("from c", ipv4);
        return 0;
    }

    public native int startVpn();

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // The handler is only used to show messages.
        if (mHandler == null) {
            mHandler = new Handler(this);
        }

        startVpn();

        // Stop the previous session by interrupting the thread.
        if (mThread != null) {
            mThread.interrupt();
        }

        if (mInterface != null) {
            try {
                mInterface.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        // Start a new session by creating a new thread.
        mThread = new Thread(this, "Top_VPN_Thread");
        Log.d("create", "create success");
        mThread.start();
        return START_STICKY;

    }

    /**
     * Called by the system to notify a Service that it is no longer used and is being removed.  The
     * service should clean up any resources it holds (threads, registered
     * receivers, etc) at this point.  Upon return, there will be no more calls
     * in to this Service object and it is effectively dead.  Do not call this method directly.
     */
    @Override
    public void onDestroy() {
        stopVPNService();
    }

    public void stopVPNService() {

        if (mThread != null) {
            mThread.interrupt();
        }
        try {
            if (mInterface != null) {
                mInterface.close();
            }
        } catch (IOException ie) {
            ie.printStackTrace();
        }
        mThread = null;
        mInterface = null;
    }

    @Override
    public boolean handleMessage(Message message) {
        //  显示提示信息
        if(message != null) {
            Toast.makeText(this, message.what, Toast.LENGTH_SHORT).show();
        }
        return false;
    }

    /**
     * When an object implementing interface <code>Runnable</code> is used
     * to create a thread, starting the thread causes the object's
     * <code>run</code> method to be called in that separately executing
     * thread.
     * <p>
     * The general contract of the method <code>run</code> is that it may
     * take any action whatsoever.
     *
     * @see Thread#run()
     */
    @Override
    public synchronized void run() {

    }
}
