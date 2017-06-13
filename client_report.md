---
title: Ipv4 over Ipv6隧道协议实验客户端报告
fontsize: 12pt
documentclassses: extreport
date: 2017-05-23
keywords: 
- 4over6
- VPN
- 计算机网络
author: 
- 计41 张盛豪 2014011450 
- 计41 李永斌 2014011442
- 计41 陈伟楠 2014011452
tags:
- 计算机网络
---


# 实验目的


- 掌握Android下应用程序开发环境的搭建和使用
- 掌握IPv4 over IPv6隧道的工作原理

<!--more-->

# 客户端实验要求

在安卓设备上实现一个4over6隧道系统的客户端程序

- 实现安卓界面程序，显示隧道报文收发状态(java语言)；
- 启用安卓VPN服务(java语言)；
- 实现底层通信程序，对4over6隧道系统控制消息和数据消息的处理(C语言)。

# 实验内容

## 前台是java语言的显示界面
- 进行网络检测并获取上联物理接口IPV6地址；
- 启动后台线程；
- 开启定时器刷新界面；
- 界面显示网络状态；
- 开启安卓VPN服务。

## 后台是C语言客户端与4over6隧道服务器之间的数据交互
- 连接服务器；
- 获取下联虚接口IPV4地址并通过管道传到前台；
- 获取前台传送到后台的虚接口描述符；
- 读写虚接口；
- 对数据进行解封装；
- 通过IPV6套接字与4over6隧道服务器进行数据交互；
- 实现保活机制，定时给服务器发送keeplive消息。

# 实验原理

## 面向Android终端的隧道原理

![4over6隧道原理](https://d2ppvlu71ri8gs.cloudfront.net/items/122w2Z0a3S2p0X0J0J0k/Image%202017-06-13%20at%206.16.41%20下午.png)

在4over6隧道中，客户端首先向过渡网关请求分配IPv4内网地址；过渡网关分配IPv4内网地址，并提供对应的IPv6网络地址；接着，安卓客户端发送4over6报文，过渡网关接受报文并进行分析，得到源地址和目的地址，将IPv4地址转化为公网地址，发送到公网之中；当过渡网关收到公网的IPv4报文之后，根据记录好的映射关系，重新封装成4over6报文，发给对应的内网用户，完成数据的转发和接受。

本实验中分别完成了过渡网关和客户端的功能，本报告主要阐述客户端的相关实现。

客户端用户处于IPV6网络环境，通过过渡网关完成IPV4网络的访问，过渡网关横跨IPV4和IPV6,提供地址转换和数据包的分发。

## VPN Service 原理

在客户端实现过程中，使用了VPNService API, 打开VPN服务后，Android系统通过iptables使用NAT将所有数据包转发到TUN虚拟网络设备，通过`mInterface.getFd();`可以获取到虚接口描述符，从而通过读虚接口获取系统的IP数据包，再将IPV4数据包封装经IPV6 socket转发给服务器端即可；服务器端根据网络请求信息完成访问，并将结果通过IPV6 socket发回，后台解析取出IPV4数据包，写入虚接口以实现数据接收。


# 具体实现

客户端实现主要分前台和后台，前台是Java实现的Android客户端显示界面，后台主要是C++ 实现的客户端与服务器端的数据交互。前后台通过读写管道以及JNI函数调用实现交互。总体流程如下：

![总体流程图](https://d2ppvlu71ri8gs.cloudfront.net/items/1D361n3e2p1w1A3E2s0X/Image%202017-06-13%20at%208.04.59%20下午.png)

## 前端实现内容

### 前端流程及完成的工作

![前端详细流程](https://d2ppvlu71ri8gs.cloudfront.net/items/0N1H1j1Z0W0N2V1T0e0B/Image%202017-06-13%20at%208.09.09%20下午.png)

前台流程如上图所示, 主要完成以下操作

- 开启定时器之前，创建一个读取IP信息管道的全局标志位flag，默认置0；
- 开始读取管道，首先读取IP信息管道，判断是否有后台传送来的IP等信息；
- 假如没有，下次循环继续读取；
- 有IP信息，就启用安卓VPN服务(此部分在后面有详细解释)；
- 把获取到的安卓虚接口描述符写入管道传到后台；
- 把flag置1，下次循环不再读取该IP信息管道；
- 读取流量信息管道；
- 从管道读取后台传来的实时流量信息；
- 把流量信息进行格式转换；
- 显示到界面；
- 界面显示的信息有运行时长、上传和下载速度、上传总流量和包数、下载总流量和包数、下联虚接口V4地址、上联物理接口IPV6地址。

### 具体实现

#### 界面

主界面采用了ScrollView嵌套LinearLayout的布局方式，界面中中有2个输入框，以及一个Button,分别用于输入服务器端的IPV6地址，端口号, 以及点击按钮链接VPN。且有一个文本框用于显示当前IPV6地址（若无，提示无IPV6网络访问权限）链接VPN之后，IPV6地址和端口输入框消失，显示一个TextView（用于显示运行时长、上传和下载速度、上传总流量和包数、下载总流量和包数、下联虚接口V4地址、上联物理接口IPV6地址等信息）和断开连接按钮。

![未连接界面](https://d2ppvlu71ri8gs.cloudfront.net/items/033l0T2m0W2i3N2x3O2D/S70613-203748.jpg)

![连接VPN后界面](https://d2ppvlu71ri8gs.cloudfront.net/items/1s0C1p091s3b0O201r3R/S70613-204410.jpg)

#### UI主线程

-   客户端开启后，即检查当前网络环境是否支持IPV6访问，若不可访问IPV6网络则用户点击【链接VPN】按钮无效。检查代码如下：
    
    ```java
    static String getIPv6Address(Context context) {
        if(! isWIFIConnected(context)) {
            return null;
        }
        try {
            final Enumeration<NetworkInterface> e = NetworkInterface.getNetworkInterfaces();
            while (e.hasMoreElements()) {
                final NetworkInterface networkInterface = e.nextElement();
                for (Enumeration<InetAddress> enumAddress = networkInterface.getInetAddresses();
                     enumAddress.hasMoreElements(); ) {
                    InetAddress inetAddress = enumAddress.nextElement();
                    if (!inetAddress.isLoopbackAddress() && !inetAddress.isLinkLocalAddress()) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        } catch (SocketException e) {
            Log.e("NET", "无法获取IPV6地址");
        }
        return null;
    }
    ```

-   为【链接VPN】按钮注册监听服务，若可访问IPV6网络（即有IPV6地址），则用户点击按钮后，启动VPN服务，并将用户填入的服务器IPV6地址和端口号通过Intent传入
-   启动VPN服务后，主界面定时刷新，通过JNI java调用C函数方式读取流量收发信息并显示在主界面。

#### VPNService

继承一个VpnService的类，启动后，先读取MainActivite中传入的Intent，获取服务器端的IPV6地址和端口号, 通过JNI java调用C函数的方式将该数据传给C后台，C后台自动与服务器端建立IPV6 Socket, 然后VPNService 循环查询C后台是否获取到服务器发回的101数据信息，得到C后台传入的IP地址，DNS，路由，以及IPV6 Socket标识信息，据此初始化VPN服务并启动，获取到TUN虚接口的文件描述符后，通过JNI函数调用传给C后台，至此前端的任务完成。

```java
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
int fd = c
send_fd(fd, PIPE_DIR);
if (!protect(Integer.parseInt(sockfd))) {
    throw new IllegalStateException("Cannot protect the mTunnel");
}
start = true;
Log.e(TAG, "configure: end");
```


## 后台实现内容

### 后台实现流程及内容


1. 创建IPV6套接字；
1. 连接4over6隧道服务器；
1. 开启定时器线程（间隔1秒）：
    1. 读写虚接口的流量信息写入管道；
    1. 获取上次收到心跳包距离当前时间的秒数S；
    1. 假如S大于60，说明连接超时，就关闭套接字；
    1. S小于60就每隔20秒给服务器发送一次心跳包。
1. 发送消息类型为100的IP请求消息；
1. while循环中接收服务器发送来的消息，并对消息类型进行判断；
    1. 101类型(IP响应)：
        1. 取出数据段，解析出IP地址，路由，DNS；
        1. 把解析到的IP地址，路由，DNS写入管道；
        1. 从管道读取前台传送来的虚接口文件描述符；
    1. 创建读取虚接口线程：
        1. 持续读取虚接口；
        1. 记录读取的长度和次数；
        1. 封装102(上网请求)类型的报头；
        1. 通过IPV6套接字发送给4over6隧道服务器。
    1. 103类型(上网应答)：
        1. 取出数据部分；
        1. 写入虚接口；
        1. 存下写入长度和写入次数。
    1. 104类型(心跳包)：
        1. 记录当前时间到一个全局变量。

![后台实现流程](https://d2ppvlu71ri8gs.cloudfront.net/items/2s3q3K3O2N2g0g332v2F/Image%202017-06-13%20at%209.17.29%20下午.png)

### 具体实现

#### 与服务器建立Socket连接

后台使用C++ Socket编程实现，用户点击【连接VPN】按钮之后即启动MyVPNService服务，同时调用C后台的`start_VPN`函数，该函数是后台主函数，MyVPNService服务通过`public native int send_addr_port(String addr, int port);`函数向C后台传递服务器端IPV6地址和端口，后台检测到取得IPV6地址信息后即完成与服务器的Socket连接及绑定。

```c
if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
    LOGE("can't create socket");
}
server.sin6_family = AF_INET6;
server.sin6_port = htons(SERVER_PORT);
Inet_pton(AF_INET6, SERVER_IPV6, &server.sin6_addr);
int temp;
if ((temp  = connect(sockfd, (struct sockaddr *) &server, sizeof(server)) )== -1){
    __android_log_print(ANDROID_LOG_ERROR, TAG, "can't access server %s", strerror(errno));
    return -1;
}
```

后台主线程中新建了3个线程`manage_data`, `readTun`以及`send_heart`, 分别用于处理数据请求，读取虚接口并发送102上网请求, 以及发送定时心跳包

#### manage_data线程

在实验中我们重新定义了Message结构体，每次从socket读取数据时均先读取`Msg_Hdr`, 然后根据length字段长度读取相应的data字段

```c
struct Msg_Hdr {
    uint32_t length; // payload 长度,不包括type, 注意协议切割
    char type; //
};

struct Msg{
    struct Msg_Hdr hdr;
    char data[MAX_MESSAGE_LENGTH];
};
```

只要与服务器socket连接保持，则`manage_data`线程循环运行，在`manage_data`线程中主要完成以下操作:

-   判断心跳包是否超时，超时则修改对应状态，并关闭socket,通知java前端VPN已断开连接
-   从socket中读取信息结构体的Msg_Hdr部分数据，根据读取到的type和length字段决定是否继续读取data
-   根据type字段做对应的处理

    ```c
    n = read(fd, &msg, needbs);
    if(n < 0) {
        LOGE("read sockfd %d error: %s \n",fd,strerror(errno));
        kill_myself();
        return -1;
    }
    else if(n == 0) {
        LOGE("recv 0 byte from server, close sockfd %d \n",fd);
        kill_myself();
        return -1;
    }
    else if(n == needbs){
        process_payload:
        char* ipv4_payload = msg.data;
        if(msg.hdr.type != 100 && msg.hdr.type != 104) {
            n = read(fd, ipv4_payload, msg.hdr.length);
            if(n != msg.hdr.length) {
                LOGE("read payload error, need %d byte, read x byte\n",msg.hdr.length);
                if(n <= 0) {
                    LOGE("读取data出错，关闭");
                    kill_myself();
                    return -1;
                }
            }
            while(n < msg.hdr.length)
                n += read(fd, ipv4_payload + n, msg.hdr.length-n);
        }

        switch(msg.hdr.type){
            case 101:
                LOGE("get 101");
                recv_ipv4_addr(&msg);
                break;
            case 103:
                LOGE("get 103");
                recv_ipv4_packet(&msg);
                break;
            case 104:
                // 心跳包,记录接收时间
                s = time(NULL);
                LOGE("%s %ld", "收到心跳包104", s);
                break;
            default:
                return -1;
        }
    }
    else {// 读到长度小于头长度说明可能出错(也有可能粘包,继续读取)
        while (n < needbs)
            n += read(fd, ((char*)&msg) + n , needbs-n);
        goto process_payload;
    }
    ```


-   读取到103上网回应包`recv_ipv4_packet`:当收到103数据包，就代表收到了服务器转发的数据，这时候直接将其data字段写入tun虚接口之中即可，并更新统计信息。

    ```c
    int write_tun(char* payload, uint32_t len) {
        // 写虚接口，收到信息，写入虚接口
        byte_in += len + 8;
        packet_in += 1;
        total_byte += len + 8;
        total_packet += 1;
        Write_nByte(tun_des, payload, len);
        return 0;
    }

    void recv_ipv4_packet(Msg* msg) {
        write_tun(msg->data, msg->hdr.length);
        debugPacket_recv(msg, msg->hdr.length);
    }
    ```

-   收到104心跳包时，更新收到心跳包的实际即可


# 实验结果及分析

实验测试阶段，我们使用魅族和华为安卓手机进行了测试，网络环境为宿舍的Tsinghua无线网（可访问IPV6）以及实验室的DIVI网络， 开始VPN后，可以正常连接，测试了多种网络传输对象和环境：

- 浏览器打开网页，网页加载速度流畅
- 斗鱼直播，视频播放流畅
- 微信，qq文字消息，表情包消息等发送接收正常

经过测试可以验证实现基本正确，且我们也测试过其稳定性，在IPV6网络稳定条件下，VPN服务不会异常中断，且手动断开VPN后可重新点击连接VPN按钮正常连接。



# 遇到的问题
## 连接VPN之后无法与服务器之间进行通信
C后台请求到IPV4地址信息之后，前端建立好VPN服务之后读取虚接口向服务器发送数据流量信息时服务器收不到相应的数据包，经过检查发现在VPN的建立过程中未对C后台Socket数据进行保护，导致后台与服务端之间的ipv6 socket链接也转发到了VPN的虚接口，这样就导致102的上网请求数据包无法发送给服务器，修改方案是利用VPNService类里的protect方法保护自己的socket。

```java
if (!protect(Integer.parseInt(sockfd))) {
    throw new IllegalStateException("Cannot protect the mTunnel");
}
```

如上所示，其中`socketfd`即为C后台与服务端链接时的Socket，通过JNI或者管道方式从C后台读取即可。


## 数据包读取不完整

在一开始的实现中有时候会发现会读取到非预设类型的报文，也就是说这些报文是读取不完整的，在传输过程中被截断或者读取时未读取完整，导致数据混乱，查阅相关资料后与服务器端一同重新定义Message结构体（见【manage_data线程】一节），并且在读取数据包时确保每次读取`sizeof(Msg_Hdr)`个字节或者`msg_hdr.length`个字节data字段，从而避免粘包现象。

```c
size_t needbs = sizeof(struct Msg_Hdr);
n = read(fd, &msg, needbs);
if(n < 0) {
    LOGE("read sockfd %d error: %s \n",fd,strerror(errno));
    kill_myself();
    return -1;
}
else if(n == 0) {
    LOGE("recv 0 byte from server, close sockfd %d \n",fd);
    kill_myself();
    return -1;
}
else if(n == needbs){
    process_payload:
    if(msg.hdr.type != 100 && msg.hdr.type != 104) {
        n = read(fd, ipv4_payload, msg.hdr.length);
        if(n != msg.hdr.length) {
            /*做出对应处理*/
        }
        while(n < msg.hdr.length)
            n += read(fd, ipv4_payload + n, msg.hdr.length-n);
    }
    /*
    * 处理读取到的数据
    */
}
else {// 读到长度小于头长度说明可能出错(也有可能粘包,继续读取)
    while (n < needbs)
        n += read(fd, ((char*)&msg) + n , needbs-n);
    goto process_payload;
}
```


## 异常退出问题

在测试时发现有时候应用程序会突然崩掉，直接被系统kill掉，输出调试信息后发现总是会报如下错误

```java
05-10 19:28:45.915 24188-24188/com.example.ipv4_over_ipv6 A/libc: Fatal signal 5 (SIGTRAP), code 1 in tid 24188 (.ipv4_over_ipv6)
```

Google之后发现是因为通过JNI方式调用的C后台的部分函数忘记了return, 而编译app时并不会检查出这种错误，而运行时检测出该问题之后则会直接kill整个程序，解决方法是添加对应的返回值语句即可。

# 实验心得体会

在这次实验中，我们掌握了Android下应用程序开发环境的搭建和使用，理解了IPV4 over IPv6的原理，掌握了Android下VPN的原理及实现，掌握了JNI调用原理，并实现了一个测试通过，效果良好，性能稳定的客户端。除此之外，我们对于原理课上所讲的IPV4 over IPv6又有了更深的理解和体会，对IPV6数据包和IPV4数据包的格式和特点有了更深入的认识，加深了对socket的理解，特别是对数据包的读取粘包问题的处理，让我们对网络包的传输有了更深入的认识。

感谢老师和助教的悉心指导。
