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

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.DatagramChannel;
import java.nio.channels.SocketChannel;

import static java.lang.Thread.sleep;

/**
 * Created by alexzhangch on 2017/4/13.
 */

public class MyVpnService extends VpnService implements Handler.Callback, Runnable{

    private static final String TAG = "Top Vpn";

    // VPN 服务器ip地址
    private String mServerAddress = "2402:f000:1:4417::900";
    private int mServerPort = 5678;



    Builder builder = new Builder();
    // 从虚接口读取数据
    private PendingIntent mConfigureIntent;
    // 用于输出调试信息 （Toast）
    private Handler mHandler;

    // 用于从虚接口读取数据
    private ParcelFileDescriptor mInterface;
    private FileInputStream mInputStream;
    private FileOutputStream mOutputStream;

    private Thread mThread;

    private DatagramChannel mTunnel = null;
    private static final int PACK_SIZE = 32767 * 2;



    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }



    public native int startVpn();
    public native boolean isGet_ip();
    public native String ip_info();

    @Override
    public void onCreate() {
        Log.e(TAG, "onCreate: before");
        super.onCreate();
        Log.e(TAG, "onCreate: after");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // The handler is only used to show messages.
        if (mHandler == null) {
            mHandler = new Handler(this);
        }
        Log.e("IMPORTANT", "before start");
        mHandler.sendEmptyMessage(R.string.debug);
        Thread temp = new Thread(new Runnable() {
            @Override
            public void run() {
                startVpn();
            }
        });
        temp.start();
        Log.e("IMPORTANT", "after start");


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
        Log.e("create", "create success");
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
            if (mInputStream != null) {
                mInputStream.close();
            }
            if (mOutputStream != null) {
                mOutputStream.close();
            }
        } catch (IOException ie) {
            ie.printStackTrace();
        }
        mThread = null;
        mInterface = null;
        mInputStream = null;
        mOutputStream = null;
        mTunnel = null;
    }

    @Override
    public boolean handleMessage(Message message) {
        //  显示提示信息
        if(message != null) {
            Toast.makeText(this, message.what, Toast.LENGTH_SHORT).show();
        }
        return false;
    }

    private void configure() throws IOException, InterruptedException {

        //  使用VPN Builder创建一个VPN接口
        // VpnService.Builder builder = new VpnService.Builder();

        String ipv4Addr;// = "13.8.0.2";
        String router;//="0.0.0.0";
        String dns1;//="59.66.16.64";
        String dns2;//="8.8.8.8";
        String dns3;//="202.106.0.20";

        while(!isGet_ip()) {
            Log.e("configure", "no ip");
            sleep(2000);
        }

        String ip_response = ip_info();
        String[] parameterArray = ip_response.split(" ");
        if (parameterArray.length < 5) {
            throw new IllegalStateException("Wrong IP response");
        }

        // 从服务器端读到的数据
        ipv4Addr = parameterArray[0];
        router = parameterArray[1];
        dns1 = parameterArray[2];
        dns2 = parameterArray[3];
        dns3 = parameterArray[4];

        builder.setMtu(1500);
        builder.addAddress(ipv4Addr, 32);


        builder.addRoute("0.0.0.0", 0); // router is "0.0.0.0" by default

        builder.addDnsServer(dns1);
        builder.addDnsServer(dns2);
        builder.addDnsServer(dns3);
        builder.setSession("Top Vpn");

        mInterface = builder.establish();

        mInputStream = new FileInputStream(mInterface.getFileDescriptor());
        mOutputStream = new FileOutputStream(mInterface.getFileDescriptor());

        //  建立一个到代理服务器的网络链接，用于数据传送
        mTunnel = DatagramChannel.open();
        // Protect the mTunnel before connecting to avoid loopback.
        if (!protect(mTunnel.socket())) {
            throw new IllegalStateException("Cannot protect the mTunnel");
        }
        // mTunnel.connect(new InetSocketAddress(mServerAddress, Integer.parseInt(mServerPort)));
        mTunnel.configureBlocking(false);
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

        mHandler.sendEmptyMessage(R.string.connecting);

        try {
            Log.e(TAG, "Starting");
            configure();
            mHandler.sendEmptyMessage(R.string.connected);
            run_vpn();

        } catch (Exception e) {
            Log.e(TAG, "Got " + e.toString());
        } finally {
            try {
                mInterface.close();
                mTunnel.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
            mHandler.sendEmptyMessage(R.string.disconnected);
        }
    }

    private boolean run_vpn() throws IOException {
        ByteBuffer packet = ByteBuffer.allocate(PACK_SIZE);

        while (true) {

            int length = mInputStream.read(packet.array());
            if (length > 0) {

                debugPacket(packet);

                //NetworkUtils.logIPPack(TAG, packet, length);
                packet.limit(length);
                // mTunnel.write(packet);
                packet.clear();
            }
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
