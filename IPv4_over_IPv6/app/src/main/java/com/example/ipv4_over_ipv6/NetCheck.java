package com.example.ipv4_over_ipv6;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Enumeration;

/**
 * 判断当前网络连接情况
 * Created by alexzhangch on 2017/4/13.
 */

class NetCheck {
    /**
     * 检查当前手机网络
     *
     * @param context
     * @return
     */
    static boolean checkNet(Context context)
    {
        // 判断连接方式
        boolean wifiConnected = isWIFIConnected(context);
        boolean mobileConnected = isMOBILEConnected(context);
        return !(!wifiConnected && !mobileConnected);
    }
    /**
     * 判断手机是否采用wifi连接
     */
    private static boolean isWIFIConnected(Context context)
    {
        // Context.CONNECTIVITY_SERVICE).
        ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (networkInfo != null && networkInfo.isConnected())
        {
            Log.e("Net", "Wifi 已连接");
            return true;
        }
        Log.e("Net", "无Wifi连接");
        return false;
    }
    private static boolean isMOBILEConnected(Context context)
    {
        ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = manager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
        if (networkInfo != null && networkInfo.isConnected())
        {
            Log.e("net", "数据信号 已连接");
            return true;
        }
        // Log.e("Net", "Mobile net not connected.");
        return false;
    }

    static Inet6Address get_IPv6_addr(Context context) throws SocketException {
        /*
        //get all local ips
        Enumeration<NetworkInterface> interfs = NetworkInterface.getNetworkInterfaces();
        Inet6Address v6_addr = null;
        while (interfs.hasMoreElements())
        {
            Log.e("interface", interfs.toString());
            NetworkInterface interf = interfs.nextElement();
            Enumeration<InetAddress> addres = interf.getInetAddresses();
            while (addres.hasMoreElements())
            {
                InetAddress in = addres.nextElement();
                if (in instanceof Inet4Address)
                {
                    Log.e("v4:", in.getHostAddress());
                }
                else if (in instanceof Inet6Address)
                {
                    v6_addr = (Inet6Address)in;
                    Log.e("v6:", in.getHostAddress());

                }
            }
        }
        */


        Inet6Address ipv6_addr = null;
        NetworkInterface nif = NetworkInterface.getByName("wlan0");

        if(nif != null) {
            for(InterfaceAddress address: nif.getInterfaceAddresses()){
                if(address.getAddress() instanceof Inet6Address){
                    ipv6_addr = (Inet6Address)(address.getAddress());
                    Log.e("get_v6", address.getAddress().getHostAddress());
                    // found IPv6 address
                    // do any other validation of address you may need here
                    break;
                }
            }

        }

        return ipv6_addr;
    }


}
