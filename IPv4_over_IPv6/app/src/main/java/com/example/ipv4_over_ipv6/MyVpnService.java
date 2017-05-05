package com.example.ipv4_over_ipv6;

import android.app.PendingIntent;
import android.content.Intent;
import android.net.VpnService;
import android.os.Binder;
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

    private static final String TAG = "Top_Vpn";

    // VPN 服务器ip地址
    private String mServerAddress = "2402:f000:1:4417::900";
    private int mServerPort = 5678;

    private String sockfd;




    // 从虚接口读取数据
    private PendingIntent mConfigureIntent;
    // 用于输出调试信息 （Toast）
    private Handler mHandler;

    // 用于从虚接口读取数据
    private ParcelFileDescriptor mInterface;
    // 虚接口描述符
    private int fd;
    private String PIPE_DIR;
    private String ROOT_DIR;


    private FileInputStream mInputStream;
    private FileOutputStream mOutputStream;

    private Thread mThread;
    private Thread cThread;

    private DatagramChannel mTunnel = null;
    private static final int PACK_SIZE = 32767 * 2;

    private boolean start = false;



    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }



    // 是否已经获得IP
    public native boolean isGet_ip();
    // 获得的IP信息
    public native String ip_info();
    // 发送数据
    public native int send_web_request(char [] data, int length);
    public native int send_web_requestt(String data, int length);
    public native int send_fd(int fd, String file);
    public native int kill();

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        Log.e(TAG, "进入startcommand");

        // The handler is only used to show messages.
        if (mHandler == null) {
            mHandler = new Handler(this);
        }
        mHandler.sendEmptyMessage(R.string.debug);


            try{
                ROOT_DIR = intent.getStringExtra("ROOT");
                PIPE_DIR = ROOT_DIR + "/tunnel";
                mServerAddress = intent.getStringExtra("SERVER_ADDR");
                mServerPort = Integer.parseInt(intent.getStringExtra("SERVER_PORT"));
            } catch (Exception e) {
                e.printStackTrace();
                Log.e("what?", "what");
                Log.e(TAG, e.toString());
            }
        // stopVPNService();

        // Start a new session by creating a new thread.
        mThread = new Thread(this, "Top_VPN_Thread");
        mThread.start();

        Log.e("create", "create success");

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
        if (cThread != null) {
            kill();
            cThread.interrupt();
        }
        try {
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
        // mInterface = null;
        // mInputStream = null;
        // mOutputStream = null;
        // mTunnel = null;
        // cThread = null;
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

        String ipv4Addr;// = "13.8.0.2";
        String router;//="0.0.0.0";
        String dns1;//="59.66.16.64";
        String dns2;//="8.8.8.8";
        String dns3;//="202.106.0.20";

        // 2. 开始读取管道，首先读取IP信息管道，判断是否有后台传送来的IP等信息
        // 3. 假如没有，下次循环继续读取；
        while(!isGet_ip()) {
        }

        // 4. 有IP信息，就启用安卓VPN服务
        String ip_response = ip_info();
        Log.e(TAG, "GET IP " + ip_response);
        String[] parameterArray = ip_response.split(" ");
        if (parameterArray.length <= 5) {
            throw new IllegalStateException("Wrong IP response");
        }

        // 从服务器端读到的IP数据
        ipv4Addr = parameterArray[0];
        router = parameterArray[1];
        dns1 = parameterArray[2];
        dns2 = parameterArray[3];
        dns3 = parameterArray[4];
        sockfd = parameterArray[5];


        Log.e(TAG, ipv4Addr + " " + router + " " + dns1 + " " + dns2 + " " + dns3 + " " + sockfd);

        Builder builder = new Builder();

        builder.setMtu(1500);
        builder.addAddress(ipv4Addr, 32);
        builder.addRoute(router, 0); // router is "0.0.0.0" by default
        builder.addDnsServer(dns1);
        builder.addDnsServer(dns2);
        builder.addDnsServer(dns3);
        builder.setSession("Top Vpn");
        Log.e(TAG, "configure: before es");

        /*try {
            mInterface.close();
            // mInterface = builder.establish();
        }catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "error" + e.toString());
            // return;
        }
        */

        try {
            // mInterface.close();
            mInterface = builder.establish();
        }catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG,"Fatal error: " + e.toString());
            return;
        }

        // 5. 把获取到的安卓虚接口描述符写入管道传到后台
        Log.e(TAG, "configure: after es");
        fd = mInterface.getFd();
        Log.e(TAG, "configure: after getfd");
        Log.e(TAG, "e" + PIPE_DIR);
        Log.e(TAG,":虚接口:" + String.valueOf(fd));
        send_fd(fd, PIPE_DIR);
        Log.e(TAG, "after send");
        // Packets received need to be written tothis output stream.
        // Java读写虚接口
        // mInputStream = new FileInputStream(mInterface.getFileDescriptor());
        // mOutputStream = new FileOutputStream(mInterface.getFileDescriptor());

        //  建立一个到代理服务器的网络链接，用于数据传送
        // mTunnel = DatagramChannel.open();
        // Protect the mTunnel before connecting to avoid loopback.
        if (!protect(Integer.parseInt(sockfd))) {
            throw new IllegalStateException("Cannot protect the mTunnel");
        }
        // mTunnel.connect(new InetSocketAddress(mServerAddress, Integer.parseInt(mServerPort)));
        // mTunnel.configureBlocking(false);

        start = true;

        Log.e(TAG, "configure: end");
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

        // mHandler.sendEmptyMessage(R.string.connecting);

        try {
            Log.e(TAG, "Starting");
            configure();
            // mHandler.sendEmptyMessage(R.string.connected);
            Log.e(TAG, "前台已完成");
            // run_vpn();
            Log.e(TAG, "end?");

        } catch (Exception e) {
            // stopVPNService();
            Log.e(TAG, "Got " + e.toString());
        } finally {
            // mHandler.sendEmptyMessage(R.string.disconnected);

        }
    }

    private boolean run_vpn() throws IOException, InterruptedException {
        // ByteBuffer packet = ByteBuffer.allocate(PACK_SIZE);
        Log.e(TAG, "前台定时读取");
        while (start) {
            // Log.e(TAG, "前台定时读取");
            // sleep(1000);
        }
        //  一个线程用于接收代理传回的数据，一个线程用于发送手机发出的数据到代理服务器
        /*
        Thread recvThread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // Allocate the buffer for a single packet.
                    ByteBuffer packet = ByteBuffer.allocate(PACK_SIZE);
                    while (true) {
                        int length = mTunnel.read(packet);
                        // mEncrypter.decrypt(packet.array(), length);
                        if (length > 0) {
                            if (packet.get(0) != 0) {
                                packet.limit(length);
                                try {
                                    mOutputStream.write(packet.array());
                                } catch (IOException ie) {
                                    ie.printStackTrace();
                                    // NetworkUtils.logIPPack(TAG, packet, length);
                                }
                                packet.clear();
                            }
                        }
                    }
                } catch (IOException ie) {
                    ie.printStackTrace();
                }
            }
        });
        recvThread.start();
        */

        // 读取本机的访问信号
        /*
        while (true) {

            int length = mInputStream.read(packet.array());
            if (length > 0) {

                // debugPacket(packet);
                // NetworkUtils.logIPPack(TAG, packet, length);
                byte[] array = packet.array();
                String data = new String(array);

                // send_web_requestt(data, length);
                Log.e(TAG, "发送完毕");
                // Log.e(TAG, data);
                // Log.e("LEN", String.valueOf(chars.length));

                //NetworkUtils.logIPPack(TAG, packet, length);
                packet.limit(length);
                // Log.e(TAG, String.valueOf(length));
                // mTunnel.write(packet);
                packet.clear();
            }

        }
        */

        return false;
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
        Log.e(TAG, "IP Version:" + version);
        Log.e(TAG, "Header Length:" + headerlength);

        String status = "";
        status += "Header Length:" + headerlength;

        buffer = packet.get(); // DSCP + EN
        buffer = packet.getChar(); // Total Length

        Log.e(TAG, "Total Length:" + buffer);

        buffer = packet.getChar(); // Identification
        buffer = packet.getChar(); // Flags + Fragment Offset
        buffer = packet.get(); // Time to Live
        buffer = packet.get(); // Protocol

        Log.e(TAG, "Protocol:" + buffer);

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

        Log.e(TAG, "Source IP:" + sourceIP);

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

        Log.e(TAG, "Destination IP:" + destIP);

        status += "   Destination IP:" + destIP;

    }
}
