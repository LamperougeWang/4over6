package com.example.ipv4_over_ipv6;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.net.VpnService;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.Inet6Address;
import java.net.SocketException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import static com.example.ipv4_over_ipv6.NetCheck.checkNet;
import static com.example.ipv4_over_ipv6.NetCheck.getIPv6Address;
import static com.example.ipv4_over_ipv6.NetCheck.get_IPv6_addr;
import static java.lang.Thread.sleep;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.

    static {
        System.loadLibrary("native-lib");
    }

    // 后台VPN主线程
    public native int startVpn();
    public native String get_info();
    public native int kill();


    public Intent vpnIntent = null;



    final private String TAG = "Main Activity";

    // c后台线程
    private Thread cThread;
    // 是否允许启动后台
    private boolean allow = false;
    // 是否已启动
    public boolean start = false;

    // 监听两个按钮
    public Button stopVpn;
    public Button startVPN;
    // 显示流量信息
    public TextView editText;

    public TextView label_addr;
    public EditText addr;
    public TextView label_port;
    public EditText port;
    public TextView label_local_ipv6;
    public EditText local_ipv6;

    private String mServerAddress = "2402:f000:5:8601:942c:3463:c810:6147";
    private String mServerPort = "6666";

    public String local_ipv6_addr = "";

    private static final int CONNECTING = 0;
    private static final int CONNECTED  = 1;
    private static final int FLUSH   = 2;
    private static final int DISCONNECTED = 3;

    public boolean stoped = false;

    public void setStart() {
        try {

            startVPN.setVisibility(View.GONE);
            label_addr.setVisibility(View.GONE);
            label_port.setVisibility(View.GONE);
            label_local_ipv6.setVisibility(View.GONE);
            addr.setVisibility(View.GONE);
            port.setVisibility(View.GONE);
            local_ipv6.setVisibility(View.GONE);
            stopVpn.setVisibility(View.VISIBLE);
            editText.setVisibility(View.VISIBLE);
        }catch (Exception e) {
            e.printStackTrace();
        }

    }

    public void setStop() {
        set_STOP();
        try {
            startVPN.setVisibility(View.VISIBLE);
            label_addr.setVisibility(View.VISIBLE);
            label_port.setVisibility(View.VISIBLE);
            label_local_ipv6.setVisibility(View.VISIBLE);
            addr.setVisibility(View.VISIBLE);
            port.setVisibility(View.VISIBLE);
            local_ipv6.setVisibility(View.VISIBLE);
            stopVpn.setVisibility(View.GONE);
            editText.setVisibility(View.GONE);
        }
        catch (Exception e) {
            e.printStackTrace();
        }


    }

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) { // in main(UI) thread
            switch (msg.what) {
                case CONNECTING:
                    editText.setText("Top VPN is connecting");
                    setStart();
                    break;
                case CONNECTED:
                    editText.setText("Connected");
                    setStart();
                    break;
                case DISCONNECTED:
                    Toast.makeText(getApplicationContext(), "Top VPN is disconnected.", Toast.LENGTH_SHORT).show();
                    setStop();
                    editText.setText("Welcome");
                    break;
                case FLUSH:
                    editText.setText(String.valueOf(msg.obj));
                    break;
                default:
                    break;
            }
        }
    };

    public boolean handleMessage(Message message) {
        //  显示提示信息
        if(message != null) {
            Toast.makeText(this, message.what, Toast.LENGTH_SHORT).show();
        }
        return false;
    }


    public interface Prefs {
        String NAME = "INFO";
        String SERVER_ADDRESS = "server.address";
        String SERVER_PORT = "server.port";
        String PIPE_DIR = "jni.dir";
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        stopVpn =  (Button) findViewById(R.id.stopVPN);
        startVPN = (Button)  findViewById(R.id.startVPN);
        // 显示流量信息
        editText = (TextView) findViewById(R.id.log);
        label_addr = (TextView) findViewById(R.id.label_address);
        addr = (EditText) findViewById(R.id.address);
        label_port = (TextView) findViewById(R.id.label_port);
        port = (EditText) findViewById(R.id.port);
        label_local_ipv6 = (TextView) findViewById(R.id.label_local_ipv6);
        local_ipv6 = (EditText) findViewById(R.id.local_address);

        // 传递服务器IP，端口等
        final SharedPreferences prefs = getSharedPreferences(Prefs.NAME, MODE_PRIVATE);
        addr.setText(prefs.getString(Prefs.SERVER_ADDRESS, mServerAddress));
        port.setText(prefs.getString(Prefs.SERVER_PORT, mServerPort));


        Log.e(TAG, getApplicationContext().getFilesDir().getPath());

        // 检查网络连接
        Toast toast = Toast.makeText(getApplicationContext(),
                "Welcome.Check Net...", Toast.LENGTH_LONG);
        toast.setGravity(Gravity.CENTER, 0, 0);
        toast.show();


        stopVpn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // 关闭VPN
                stoped = true;
                kill();
                // startService(getServiceIntent().setAction(MyVpnService.ACTION_DISCONNECT));
                Log.e(TAG, "onClick: stop");
                startService(getServiceIntent().setAction(MyVpnService.ACTION_DISCONNECT));
                stopService(getServiceIntent());
                Log.e(TAG, "onClick: after stop service");
                setStop();
            }
        });

        final Handler net_handler = new Handler();
        Runnable runnable=new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                if (checkNet(getApplicationContext())) {
                    Log.d("Net", "网络已连接");
                    String iPv6_addr = getIPv6Address(getApplicationContext());
                    if (iPv6_addr != null) {
                        // Toast.makeText(getApplicationContext(), "IPv6 网络访问正常", Toast.LENGTH_SHORT).show();
                        allow = true;
                        local_ipv6_addr = iPv6_addr;
                        local_ipv6.setText(iPv6_addr);
                    } else {
                        allow = false;
                        Toast.makeText(getApplicationContext(), "无IPv6访问权限，请检查网络, 4s后重试...", Toast.LENGTH_SHORT).show();
                        local_ipv6.setText("无IPv6访问权限，请检查网络");
                        local_ipv6_addr = "无IPv6访问权限，请检查网络";
                    }
                }
                else {
                    allow = false;
                }
                //要做的事情
                net_handler.postDelayed(this, 4000);
            }
        };

        net_handler.postDelayed(runnable, 2000);//每两秒检查一次网络runnable.

        // 点击按钮连接VPN
        startVPN.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // 关闭前一个VPN
                kill();
                // stopService(getServiceIntent());

                if(!allow) {
                    Toast.makeText(getApplicationContext(), "无IPV6网络，请联网后重试...", Toast.LENGTH_SHORT).show();
                }
                else {
                    prefs.edit()
                            .putString(Prefs.SERVER_ADDRESS, addr.getText().toString())
                            .putString(Prefs.SERVER_PORT, port.getText().toString())
                            .putString(Prefs.PIPE_DIR, getApplicationContext().getFilesDir().getPath())
                            .commit();

                    // kill();

                    // 有IPv6网络，可以连接,启动C后台
                    Runnable jni_back = new Runnable() {
                        @Override
                        public void run() {
                            stoped = false;
                            Message message = new Message();
                            message.what = CONNECTED;
                            message.obj  = null;
                            mHandler.sendMessage(message);
                            // setStart();
                            startVpn();
                            Log.e(TAG, "run: startvpn over ready stopservice");
                            stopService(getServiceIntent());
                            Log.e(TAG, "run: startvpn over after stopservice");

                            // set_STOP();
                            // setStop();
                            Message end_message = new Message();
                            end_message.what = DISCONNECTED;
                            end_message.obj  = null;
                            mHandler.sendMessage(end_message);
                        }
                    };
                    cThread = new Thread(jni_back);
                    cThread.start();

                    flush();

                    // 客户程序一般需要先调用VpnService.prepare函数
                    // 询问用户权限，检查当前是否已经有VPN连接，如果有判断是否是本程序创建的
                    Intent intent = VpnService.prepare(getApplicationContext());
                    Toast.makeText(getApplicationContext(), "Top VPN is connecting...", Toast.LENGTH_SHORT).show();
                    Log.e("Click", "Top VPN 正在连接");
                    if (intent != null) {
                        // 没有VPN连接，或者不是本程序创建的
                        Log.e(TAG, "NOT NULL");
                        startActivityForResult(intent, 0);
                    } else {
                        Log.e(TAG, "NULL");
                        onActivityResult(0, RESULT_OK, null);
                    }
                }
            }
        });

    }


    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            // 如果返回结果是OK的，也就是用户同意建立VPN连接，则将你写的，继承自VpnService类的服务启动起来就行了。
            // Intent intent = new Intent(this, MyVpnService.class);
            /*
            intent.putExtra("ROOT", getApplicationContext().getFilesDir().getPath());
            intent.putExtra("SERVER_ADDR", addr.getText().toString());
            intent.putExtra("SERVER_PORT", port.getText().toString());
            */
            startService(getServiceIntent().setAction(MyVpnService.ACTION_CONNECT));
        }
    }

    private void flush() {

        Runnable jni_back = new Runnable() {
            @Override
            public void run() {
                while (true) {
                    if(! stoped) {
                        Message message = new Message();
                        message.what = FLUSH;
                        message.obj  = get_info();
                        mHandler.sendMessage(message);
                    }


                    try {
                        sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        };

        Thread flushThread = new Thread(jni_back);
        flushThread.start();
    }



    private Intent getServiceIntent() {
        if (vpnIntent == null) {
            Log.e(TAG, "getServiceIntent: New Intent");
            vpnIntent = new Intent(this, MyVpnService.class);
        }
        return vpnIntent;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    // public native String stringFromJNI();

    public void set_STOP() {
        stoped = true;
        Message message = new Message();
        message.what = FLUSH;
        message.obj  = "Vpn 已断开";
        mHandler.sendMessage(message);
    }
}
