---
title: 安卓开发记录
date: 2017-04-12 12:52:09
tags:
    - Android
    - 4over6
    - 网络

---

# 安卓中用户权限设置

<!--more-->


位置：`AndroidManifest.xml`  在package之后， Application 之前

```xml
    <!--用户权限-->

    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
    <uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
    <!--VPN 权限-->
    <uses-permission android:name="android.permission.BIND_VPN_SERVICE" />
```


# Android中Intent机制

## 作用

-   启动 Activity：
    Activity 表示应用中的一个屏幕。通过将 Intent 传递给 startActivity()，您可以启动新的 Activity 实例。Intent 描述了要启动的 Activity，并携带了任何必要的数据。 如果您希望在 Activity 完成后收到结果，请调用 startActivityForResult()。在 Activity 的 onActivityResult() 回调中，您的 Activity 将结果作为单独的 Intent 对象接收。如需了解详细信息，请参阅 Activity 指南。
-   启动服务：
    Service 是一个不使用用户界面而在后台执行操作的组件。通过将 Intent 传递给 startService()，您可以启动服务执行一次性操作（例如，下载文件）。Intent 描述了要启动的服务，并携带了任何必要的数据。 如果服务旨在使用客户端-服务器接口，则通过将 Intent 传递给 bindService()，您可以从其他组件绑定到此服务。如需了解详细信息，请参阅服务指南。
-   传递广播：
    广播是任何应用均可接收的消息。系统将针对系统事件（例如：系统启动或设备开始充电时）传递各种广播。通过将 Intent 传递给 sendBroadcast()、sendOrderedBroadcast() 或 sendStickyBroadcast()，您可以将广播传递给其他应用。



## Intent 结构

- action -- 想要实施的动作，例: ACTION_VIEW, ACTION_EDIT, ACTION_MAIN, etc.
- data -- 具体的数据，一般由以Uri表示，例：通讯录中的某条记录，会以Uri来表示
- category -- 为实施的动作添加的额外信息，即Intent组件的种类信息，一个Intent对象可以有任意个category，例：CATEGORY_LAUNCHER 意味着，它应该在启动器中作为顶级应用而存在
- type -- 显示指定Intent的数据类型（MIME类型 - 多用途互联网邮件扩展，Multipurpose Internet Mail Extensions），例：一个组件是可以显示图片数据的而不能播放声音文件。很多情况下，data类型可在URI中找到，比如content:开头的URI，表明数据由设备上的content provider提供。但是通过设置这个属性，可以强制采用显式指定的类型而不再进行推导
MIME类型有两种：单个记录格式、多个记录格式
- component -- 指定Intent的目标组件的类名称。通常 Android会根据Intent 中包含的其它属性的信息，比如action、data/type、category进行查找，最终找到一个与之匹配的目标组件。但是，如果 component这个属性有指定的话，将直接使用它指定的组件，而不再执行上述查找过程。指定了这个属性以后，Intent的其它所有属性都是可选的，例如：Intent it = new Intent(Activity.Main.this, Activity2.class); startActivity(it);
- extras -- 附加信息，例如：it.putExtras(bundle) - 使用Bundle来传递数据；

## Builder 创建并初始化tun0虚拟网络端口
- MTU（Maximun Transmission Unit），即表示虚拟网络端口的最大传输单元，如果发送的包长度超过这个数字，则会被分包；
- Address，即这个虚拟网络端口的IP地址；
- Route，只有匹配上的IP包，才会被路由到虚拟端口上去。如果是0.0.0.0/0的话，则会将所有的IP包都路由到虚拟端口上去；
- DNS Server，就是该端口的DNS服务器地址；
- Search Domain，就是添加DNS域名的自动补齐。DNS服务器必须通过全域名进行搜索，但每次查找都输入全域名太麻烦了，可以通过配置域名的自动补齐规则予以简化；
- Session，就是你要建立的VPN连接的名字，它将会在系统管理的与VPN连接相关的通知栏和对话框中显示出来；
- Configure Intent，这个intent指向一个配置页面，用来配置VPN链接。它不是必须的，如果没设置的话，则系统弹出的VPN相关对话框中不会出现配置按钮。

最后调用Builder.establish函数，如果一切正常的话，tun0虚拟网络接口就建立完成了。并且，同时还会通过iptables命令，修改NAT表，将所有数据转发到tun0接口上。

## 虚接口数据的读取

ParcelFileDescriptor类有一个getFileDescriptor函数，其会返回一个文件描述符，这样就可以将对接口的读写操作转换成对文件的读写操作。

每次调用FileInputStream.read函数会读取一个IP数据包，而调用FileOutputStream.write函数会写入一个IP数据包到TCP/IP协议栈。

## 暂存




```
int send_request_ip(int server_socket, JNIEnv *env, jobject obj, jclass cls) {
    LOGD("请求ip地址");

    // 发送成功

    responseIPv4("166.111.68.250 255.255.255.0 233.233.233", env, obj, cls);

    return 0;


}

int responseIPv4(string ipv4, JNIEnv *env, jobject obj, jclass cls) {

    LOGD(ipv4);

    //2 寻找class里面的方法
    //   jmethodID   (*GetMethodID)(JNIEnv*, jclass, const char*, const char*);
    jmethodID method1 = (*env)->GetMethodID(env,cls,"responseIPv4","(Ljava/lang/String;)I");
    if(method1==0){
        LOGD("find method1 error");
        return 0;
    }
    LOGD("find method1 ");
    //3 .调用这个方法
    //    void        (*CallVoidMethod)(JNIEnv*, jobject, jmethodID, ...);
    (*env)->CallVoidMethod(env,obj,method1);

    return 0;
}


JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_startVpn(JNIEnv *env, jobject instance) {

    // TODO

    /*
     * 开启VPN服务
     * */

    // c调用java函数
    jclass cls;
    cls = env->GetObjectClass(instance);
    if(cls != NULL) {
        send_request_ip(0, env, instance, cls);
    }

    return 0;


}
```
