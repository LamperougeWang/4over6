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
import android.widget.TextView;
import android.widget.Toast;
import android.content.SharedPreferences;


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

    public static final String ACTION_CONNECT = "com.example.ipv4_over_ipv6.START";
    public static final String ACTION_DISCONNECT = "com.example.ipv4_over_ipv6.STOP";
    public static final String ACTION_DESTROY = "com.example.ipv4_over_ipv6.DESTROY";


    // 用于输出调试信息 （Toast）
    private Handler mHandler;

    // 用于从虚接口读取数据
    private ParcelFileDescriptor mInterface;
    private String PIPE_DIR;

    // 本线程
    private Thread mThread;


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
    public native int send_fd(int fd, String file);
    public native int kill();
    public native int send_addr_port(String addr, int port);


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        if (intent != null && ACTION_CONNECT.equals(intent.getAction())) {
            Log.e(TAG, "start service");
            connect();

            /*
            * TART_STICKY：如果service进程被kill掉，保留service的状态为开始状态，但不保留递送的intent对象。
            * 随后系统会尝试重新创建service，由于服务状态为开始状态，所以创建服务后一定会调用onStartCommand(Intent,int,int)
            * */
            return START_STICKY;
        } else {
            Log.e(TAG, "stop service");
            kill();
            stopVPNService();
            /*
            * START_NOT_STICKY：“非粘性的”。
            * 使用这个返回值时，如果在执行完onStartCommand后，
            * 服务被异常kill掉，系统不会自动重启该服务
            * */
            return START_NOT_STICKY;
        }



    }

    private void connect() {
        // 每次首先关闭之前的VPN连接
        stopVPNService();

        Log.e(TAG, "进入startcommand");

        // The handler is only used to show messages.
        if (mHandler == null) {
            mHandler = new Handler(this);
        }
        // mHandler.sendEmptyMessage(R.string.debug);

        try{

            // Extract information from the shared preferences.
            final SharedPreferences prefs = getSharedPreferences(MainActivity.Prefs.NAME, MODE_PRIVATE);
            final String server = prefs.getString(MainActivity.Prefs.SERVER_ADDRESS, "");
            String ROOT_DIR = prefs.getString(MainActivity.Prefs.PIPE_DIR, "");
            PIPE_DIR = ROOT_DIR + "/tunnel";
            final int port = Integer.parseInt(prefs.getString(MainActivity.Prefs.SERVER_PORT, ""));
            Log.e(TAG, server);

            send_addr_port(server, port);

            Log.e(TAG, "after end");

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
    }


    /**
     * Called by the system to notify a Service that it is no longer used and is being removed.  The
     * service should clean up any resources it holds (threads, registered
     * receivers, etc) at this point.  Upon return, there will be no more calls
     * in to this Service object and it is effectively dead.  Do not call this method directly.
     */
    @Override
    public void onDestroy() {
        Log.e(TAG, "onDestroy stop");
        kill();
        stopVPNService();
        stopSelf();
    }

    public void stopVPNService() {
        // kill();

        if (mThread != null) {
            mThread.interrupt();
        }
        if(mInterface != null) {
            try {
                mInterface.close();
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                mInterface = null;
            }

        }
        mThread = null;
        //stopSelf();
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
        // sockrt 描述符
        String sockfd = parameterArray[5];
        Log.e(TAG, ipv4Addr + " " + router + " " + dns1 + " " + dns2 + " " + dns3 + " " + sockfd);
        Builder builder = new Builder();
        builder.setMtu(1500);
        builder.addAddress(ipv4Addr, 32);
        builder.addRoute(router, 0); // router is "0.0.0.0" by default
        builder.addDnsServer(dns1);
        builder.addDnsServer(dns2);
        builder.addDnsServer(dns3);
        builder.setSession("Top Vpn");
        try {
            mInterface = builder.establish();
        }catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG,"Fatal error: " + e.toString());
            return;
        }
        // 5. 把获取到的安卓虚接口描述符写入管道传到后台
        int fd = mInterface.getFd();
        send_fd(fd, PIPE_DIR);
        if (!protect(Integer.parseInt(sockfd))) {
            throw new IllegalStateException("Cannot protect the mTunnel");
        }
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
            Log.e(TAG, "VPN配置已完成");
            // run_vpn();
            // flush();
            Log.e(TAG, "end?");

        } catch (Exception e) {
            Log.e(TAG, "Got " + e.toString());
        } finally {

        }
    }

    private boolean run_vpn() throws IOException, InterruptedException {
        // ByteBuffer packet = ByteBuffer.allocate(PACK_SIZE);
        Log.e(TAG, "前台定时读取");
        while (start) {
            // Log.e(TAG, "前台定时读取");
            // sleep(1000);
        }
        return false;
    }
}
