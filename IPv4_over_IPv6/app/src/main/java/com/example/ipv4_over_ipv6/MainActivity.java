package com.example.ipv4_over_ipv6;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
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

    final private String TAG = "Main Activity";

    private boolean start = false;
    private MyVpnService mVPNService;
    // c后台线程
    private Thread cThread;

    // private String server_addr;
    // private String server_port;

    EditText addr;
    EditText port;



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        addr = (EditText) findViewById(R.id.address);
        port = (EditText) findViewById(R.id.port);

        Log.e(TAG, getApplicationContext().getFilesDir().getPath());

        // 检查网络连接
        Toast toast = Toast.makeText(getApplicationContext(),
                "正在检查网络", Toast.LENGTH_LONG);
        toast.setGravity(Gravity.CENTER, 0, 0);
        toast.show();




        final Handler net_handler = new Handler();
        Runnable runnable=new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                if (checkNet(getApplicationContext())) {
                    Log.d("Net", "网络已连接");
                    String iPv6_addr = getIPv6Address();
                    EditText local_ipv6 = (EditText) findViewById(R.id.local_address);
                    if (iPv6_addr != null) {
                        // Toast.makeText(getApplicationContext(), "IPv6 网络访问正常", Toast.LENGTH_SHORT).show();

                        local_ipv6.setText(iPv6_addr);
                    } else {
                        local_ipv6.setText("无IPv6访问权限，请检查网络");
                    }
                }


                //要做的事情
                net_handler.postDelayed(this, 2000);
            }
        };

        net_handler.postDelayed(runnable, 2000);//每两秒执行一次runnable.



        // 点击按钮连接VPN
        final Button startVPN;
        startVPN = (Button) findViewById(R.id.startVPN);
        startVPN.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                start = !start;

                    Runnable jni_back = new Runnable() {
                        @Override
                        public void run() {
                            startVpn();
                        }
                    };

                    cThread = new Thread(jni_back);
                    cThread.start();


                    Log.e("click", "click");


                    // 客户程序一般需要先调用VpnService.prepare函数
                    // 询问用户权限，检查当前是否已经有VPN连接，如果有判断是否是本程序创建的
                    Intent intent = VpnService.prepare(getApplicationContext());
                    // Toast.makeText(getApplicationContext(), "Top VPN is connecting...", Toast.LENGTH_SHORT).show();
                    Log.e("Click", "Top VPN 正在连接");
                    if (intent != null) {
                        // 没有VPN连接，或者不是本程序创建的
                        Log.e(TAG, "NULL");
                        startActivityForResult(intent, 0);
                    } else {
                        Log.e(TAG, "NOT NULL");
                        onActivityResult(0, RESULT_OK, null);
                    }


            }
        });

        // Example of a call to a native method
        // TextView tv = (TextView) findViewById(R.id.sample_text);
        // tv.setText(stringFromJNI());
    }


    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            // 如果返回结果是OK的，也就是用户同意建立VPN连接，则将你写的，继承自VpnService类的服务启动起来就行了。
            Intent intent = new Intent(this, MyVpnService.class);
            intent.putExtra("ROOT", getApplicationContext().getFilesDir().getPath());
            intent.putExtra("SERVER_ADDR", addr.getText().toString());
            intent.putExtra("SERVER_PORT", port.getText().toString());
            startService(intent);
            /*
            if(start) {
                bindService(intent, mServiceConnection, Context.BIND_AUTO_CREATE);
            }
            */
            // start = false;

        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    // public native String stringFromJNI();
}
