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
import java.nio.ByteBuffer;

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
    // 从虚接口读取数据
    private PendingIntent mConfigureIntent;
    // 用于输出调试信息 （Toast）
    private Handler mHandler;
    private ParcelFileDescriptor mInterface;

    private Thread mThread;


    // Used to load the 'vpn_service' library on application startup.
    static {
        System.loadLibrary("native-lib");
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
        try {
            Log.i(TAG, "Starting");
            InetSocketAddress server = new InetSocketAddress(mServerAddress,
                    mServerPort);

            run(server);

        } catch (Exception e) {
            Log.e(TAG, "Got " + e.toString());
            try {
                mInterface.close();
            } catch (Exception e2) {
                // ignore
            }
            Message msgObj = mHandler.obtainMessage();
            msgObj.obj = "Disconnected";
            mHandler.sendMessage(msgObj);

        } finally {

        }
    }


    private void debugPacket(ByteBuffer packet) {
		/*
		 * for(int i = 0; i < length; ++i) { byte buffer = packet.get();
		 *
		 * Log.d(TAG, "byte:"+buffer); }
		 */

        int buffer = packet.get();
        int version;
        int headerlength;
        version = buffer >> 4;
        headerlength = buffer & 0x0F;
        headerlength *= 4;
        Log.d(TAG, "IP Version:" + version);
        Log.d(TAG, "Header Length:" + headerlength);

        String status = "";
        status += "Header Length:" + headerlength;

        buffer = packet.get(); // DSCP + EN
        buffer = packet.getChar(); // Total Length

        Log.d(TAG, "Total Length:" + buffer);

        buffer = packet.getChar(); // Identification
        buffer = packet.getChar(); // Flags + Fragment Offset
        buffer = packet.get(); // Time to Live
        buffer = packet.get(); // Protocol

        Log.d(TAG, "Protocol:" + buffer);

        status += "  Protocol:" + buffer;

        buffer = packet.getChar(); // Header checksum

        String sourceIP = "";
        buffer = packet.get(); // Source IP 1st Octet
        sourceIP += buffer;
        sourceIP += ".";

        buffer = packet.get(); // Source IP 2nd Octet
        sourceIP += buffer;
        sourceIP += ".";

        buffer = packet.get(); // Source IP 3rd Octet
        sourceIP += buffer;
        sourceIP += ".";

        buffer = packet.get(); // Source IP 4th Octet
        sourceIP += buffer;

        Log.d(TAG, "Source IP:" + sourceIP);

        status += "   Source IP:" + sourceIP;

        String destIP = "";
        buffer = packet.get(); // Destination IP 1st Octet
        destIP += buffer;
        destIP += ".";

        buffer = packet.get(); // Destination IP 2nd Octet
        destIP += buffer;
        destIP += ".";

        buffer = packet.get(); // Destination IP 3rd Octet
        destIP += buffer;
        destIP += ".";

        buffer = packet.get(); // Destination IP 4th Octet
        destIP += buffer;

        Log.d(TAG, "Destination IP:" + destIP);

        status += "   Destination IP:" + destIP;

    }
}
