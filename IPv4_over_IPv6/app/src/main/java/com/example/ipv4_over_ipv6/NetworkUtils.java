package com.example.ipv4_over_ipv6;
import android.util.Log;

import java.nio.ByteBuffer;
/**
 * Created by alexzhangch on 2017/5/4.
 */

public class NetworkUtils {
    public static String byteArrayToHexString(byte[] src, int length) {

        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            stringBuilder.append(' ');
            if (hv.length() < 2) {
                stringBuilder.append('0');
            }
            stringBuilder.append(hv);
        }
        String result =  stringBuilder.toString();
        Log.e("LENGTH", length + " " +  result.length());

        return result;
    }



    public static long byteToLong(byte[] bytes, int start) {

        return (bytes[start + 3] & 0xff) | ((bytes[start + 2] & 0xff) << 8) |
                ((bytes[start + 1] & 0xff) << 16) |
                ((bytes[start] & 0xff) << 24);
    }

    public static String longToAddressString(final long ip) {
        final long[] mask = {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000};
        final StringBuilder ipAddress = new StringBuilder();
        for (long i = 0; i < mask.length; i++) {
            long part = (ip & mask[(int) i]) >> (i * 8);
            if (part < 0) {
                part = 256 + part;
            }
            ipAddress.insert(0, part);
            if (i < mask.length - 1) {
                ipAddress.insert(0, ".");
            }
        }
        return ipAddress.toString();
    }

    public static void logIPPack(String TAG, ByteBuffer packet, int length){

        Log.e(TAG, "\n" + NetworkUtils.byteArrayToHexString(packet.array(), length));
        String sourceAddress = NetworkUtils.longToAddressString(NetworkUtils.byteToLong(packet.array(), 12));
        String destAddress = NetworkUtils.longToAddressString(NetworkUtils.byteToLong(packet.array(), 16));
        Log.e(TAG, "from " + sourceAddress + " to " + destAddress);
    }
}
