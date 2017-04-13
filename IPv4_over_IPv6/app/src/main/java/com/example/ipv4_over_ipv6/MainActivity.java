package com.example.ipv4_over_ipv6;

import android.content.Intent;
import android.net.VpnService;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        // 客户程序一般需要先调用VpnService.prepare函数
        // 询问用户权限，检查当前是否已经有VPN连接，如果有判断是否是本程序创建的

        Intent intent = VpnService.prepare(this);
        if (intent != null) {
            // 没有VPN连接，或者不是本程序创建的
            startActivityForResult(intent, 0);
        } else {
            onActivityResult(0, RESULT_OK, null);
        }

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
    }


    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            // 如果返回结果是OK的，也就是用户同意建立VPN连接，则将你写的，继承自VpnService类的服务启动起来就行了。
            Intent intent = new Intent(this, MyVpnService.class);
            startService(intent);
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
