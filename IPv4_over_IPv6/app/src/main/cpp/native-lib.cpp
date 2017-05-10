
#include "unp.h"
extern "C"

using namespace std;



#define big_size 2014
#define small_size 50
#define MAXBUF 10240
#define MAX_FIFO_BUF 1024
#define MAX_MESSAGE_LENGTH 4096


#define VPN_SERVER_IPV6_ADDRESS "2402:f000:1:4417::900"
#define VPN_SERVER_TCP_PORT 5678

#define VPN_SERVER_IPV6_ADDRESS_TEST "2402:f000:5:8601:942c:3463:c810:6147"
// #define VPN_SERVER_IPV6_ADDRESS_TEST "2402:f000:5:8601:8e23:e6e3:b7:3110"
#define VPN_SERVER_TCP_PORT_TEST 6666

// 客户端消息结构体
struct Message {
    int length;                             // 整个结构体的字节长度
    char type;                              // 类型
    char data[MAX_MESSAGE_LENGTH];          // 数据段
};

struct Msg_Hdr {
    uint32_t length; // payload 长度,不包括type, 注意协议切割
    char type; //
};

struct Msg{
    struct Msg_Hdr hdr;
    char data[MAX_MESSAGE_LENGTH];
};


struct Ipv4_Request_Reply{
    struct in_addr addr_v4[5];
};

in_addr addr_v4[5];

// 100 IP地址请求
const char IP_REQUEST = 100;
// 101 IP地址回应
const char IP_RESPONSE = 101;
// 102 上网请求
const char WEB_REQUEST = 102;
// 103 上网回应
const char WEB_RESPONSE = 103;
// 104 心跳包
const char HEART_BEAT = 104;

// VPN是否已连接
bool connected = false;
// 是否得到101包
bool get_ip  = false;
// 获取到的ip数据
char toWrite[MAX_MESSAGE_LENGTH];
// 统计数据
char packet_log[MAX_MESSAGE_LENGTH];
// 链接服务器的socket
int sockfd;
// 是否超时
bool over_time = false;
// 存活标记
int live_flag = 0;
//time_t是long类型，精确到秒，是当前时间和1970年1月1日零点时间的差
time_t s = time(NULL);
time_t start_time;

// 虚接口描述符
int tun_des;
char  ip_[5][16];

bool test = true;

char * PIPE_DIR;

// 数据包记录
int byte_in = 0;
int byte_out = 0;
int total_byte = 0;
int packet_in = 0;
int packet_out = 0;
int total_packet = 0;
bool kill_it = false;

// 连接信息
char server_ip[MAX_MESSAGE_LENGTH];
char ipv4[MAX_MESSAGE_LENGTH];


void createMessage(struct Message* msg, char type, char* data, int length)
{
    msg->type = type;
    memset(&(msg->data), 0, sizeof(msg->data));
    if (data == NULL) {
        length = 0;
    } else // (data != NULL)
    {
        // memset(&(msg->data), 0, sizeof(msg->data));
        memcpy(&(msg->data), data, length);
    }
    if(test) {
        msg->length = length;

    } else{
        msg->length = 5+length;
    }
}

void request_ipv4() {
    struct Message send_msg;

    // 创建发送数据包
    // send_msg.length = 5;
    // send_msg.type = IP_REQUEST;
    // memset(&(send_msg.data), 0, sizeof(send_msg.data));

    createMessage(&send_msg, IP_REQUEST, NULL, 0);

    // 向服务器发送数据
    if (send(sockfd, &send_msg, sizeof(send_msg), 0) < 0)
    {
        // close(sockfd);
        
        // fprintf(stderr, "发送失败\n");
        LOGE("IPv4请求失败");
    }

    LOGE("%s", "IPV4地址请求");

    struct Msg msg;

    /*if(test) {
        ssize_t needbs = sizeof(struct Msg_Hdr);

        memset(&msg, 0, sizeof(struct Msg));
        if(read(sockfd, &msg, needbs) < 0) {
            LOGE("%s", "can't read header");
        }

        if(msg.hdr.type == IP_RESPONSE) {
            if(read(sockfd, msg.ipv4_payload, msg.hdr.length) < 0) {
                __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", "can't read 101 reply payload") ;
            }
            char  ip_[5][16];
            Ipv4_Request_Reply* reply = (Ipv4_Request_Reply*)msg.ipv4_payload;
            for(int i = 0 ; i < 5; ++i) {
                char buf[16];
                inet_ntop(AF_INET, &(reply->addr_v4[i]), ip_[i], sizeof(ip_[i]));
            }
            
            sprintf(toWrite, "%s %s %s %s %s %d", ip_[0], ip_[1], ip_[2], ip_[3], ip_[4], sockfd);
            get_ip = true;

            LOGE("收到 %s", toWrite);
        }

    }*/

}


void * send_heart(void *arg) {
    struct Msg send_msg;

    send_msg.hdr.length = 0;
    send_msg.hdr.type = HEART_BEAT;
    memset(&(send_msg.data), 0, sizeof(send_msg.data));
    // 创建发送数据包
    /*
    createMessage(&send_msg, HEART_BEAT, NULL, 0);
    send_msg.length = 0;
    send_msg.type = HEART_BEAT;
    memset(&(send_msg.data), 0, sizeof(send_msg.data));
    printf("%du\n", sizeof(send_msg));
     */
    while(!over_time && connected) {
        // printf("发送心跳包%ld\n", time(NULL));

        // 向服务器发送数据
        if (send(sockfd, &send_msg, sizeof(struct Msg_Hdr), 0) < 0)
        {
            // close(sockfd);
            LOGE("发送失败\n");
            sleep(20);
            continue;
        }
        LOGE("%s %ld", "发送心跳包", time(NULL));

        sleep(20);
    }
    


    return NULL;
}

int debugPacket(struct Msg *c_msg,int length) {
    /*
    iphdr* ipv4hdr = (iphdr*)(data);
    //获取目的地址

    struct sockaddr_in dstaddr,srcaddr;
    dstaddr.sin_addr.s_addr = ipv4hdr->daddr;
    dstaddr.sin_family = AF_INET;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    srcaddr.sin_addr.s_addr = ipv4hdr->saddr;
    srcaddr.sin_family = AF_INET;

    // ssize_t n = sendto(rawfd, c_msg->ipv4_payload, c_msg->hdr.length, 0, (SA*)&dstaddr, addr_len);
    char buf[512],buf2[512];
    Inet_ntop(AF_INET, &(dstaddr.sin_addr), buf,sizeof(buf));
    Inet_ntop(AF_INET, &(srcaddr.sin_addr), buf2,sizeof(buf2));
    
    LOGE("from %s to %s\n", buf2, buf);
    */

    char hex[MAX_MESSAGE_LENGTH * 100];
    sprintf(hex, "%s", "发送 ");
    for(int i = 0 ; i < c_msg->hdr.length; ++i) {
        uint32_t temp = 0;
        temp = c_msg->data[i];
        // LOGE("%02x i:%d ",(temp),i);
        sprintf(hex, "%s %02x", hex, (temp));
    }
    LOGE("%d %s", length, hex);
    LOGE("ready");
    return 0;
    
}

char * get_log(){

    if(connected) {

        // 已运行的时间
        time_t run_time;
        time(&run_time);
        struct tm * start_info;
        struct tm * run_info;
        long t = run_time - start_time;
        run_info = gmtime(&t);
        // VPN开启时间
        start_info = localtime(& start_time);

        sprintf(packet_log, "已运行:%02d:%02d:%02d\n", run_info->tm_hour, run_info->tm_min, run_info->tm_sec);

        sprintf(packet_log, "%s已发送:%d B/%d 个数据包\n已接收:%dB/%d个数据包\n共出入:%dB/%d个数据包", packet_log, byte_out, packet_out, byte_in, packet_in, total_byte, total_packet );
    }
    else {
        sprintf(packet_log, "Top Vpn is connecting....");
    }

    LOGE("%s", packet_log);
    return packet_log;
}

void * readTun(void *arg) {
    // 读取虚接口的线程，while循环反复读取

    struct Msg send_msg;
    // 虚接口读出的数据包
    char packet[MAX_MESSAGE_LENGTH];

    while(connected) {
        // 持续读取虚接口

        // LOGE("readTun");

        memset(&send_msg, 0, sizeof(send_msg));
        memset(packet, 0, sizeof(packet));

        // 封装102请求包
        int length = read(tun_des, packet, sizeof(packet));
        if(length > 0) {
            // 封装数据
            // LOGE("read from tunnel %d %s", length, packet);
            // debugPacket(packet);

            send_msg.hdr.type = WEB_REQUEST;
            send_msg.hdr.length = length;
            memcpy(send_msg.data, packet, length);
            // 发送IPV6数据包
            int temp = write(sockfd, (char*)&send_msg, length + sizeof(struct Msg_Hdr));
            if(temp < 0) {
                LOGE("102 发送失败");
                continue;
            }



            LOGE("102 发送成功 %d msg.length: %d write_length:%d %d %d %s", send_msg.hdr.type, send_msg.hdr.length, temp, sizeof(struct Msg_Hdr), length + sizeof(struct Msg_Hdr), send_msg.data);
            // 记录读取的长度和次数
            debugPacket(&send_msg, send_msg.hdr.length);
            byte_out += length + 5;
            packet_out += 1;
            total_byte += length + 5;
            total_packet += 1;

            get_log();
        }
    }
}



int debugPacket_recv(struct Msg *c_msg,int length) {

    char hex[MAX_MESSAGE_LENGTH * 100];
    sprintf(hex, "%s", "收到 ");
    for(int i = 0 ; i < c_msg->hdr.length; ++i) {
        uint32_t temp = 0;
        temp = c_msg->data[i];
        // LOGE("%02x i:%d ",(temp),i);
        sprintf(hex, "%s %02x", hex, (temp));
    }
    LOGE("%d %s", length, hex);
    LOGE("ready");
    return 0;
}


int write_tun(char* payload, uint32_t len) {
    // 写虚接口，收到信息，写入虚接口
    byte_in += len + 8;
    packet_in += 1;
    total_byte += len + 8;
    total_packet += 1;
    // LOGE("before write");
    Write_nByte(tun_des, payload, len);
    // LOGE("after write");
    return 0;
}


void recv_ipv4_packet(Msg* msg) {
    write_tun(msg->data, msg->hdr.length);
    debugPacket_recv(msg, msg->hdr.length);
    LOGE("after");
}

void recv_ipv4_addr(Msg* msg) {
    Ipv4_Request_Reply* reply = (Ipv4_Request_Reply*)msg->data;
    for(int i = 0 ; i < 5; ++i) {
        inet_ntop(AF_INET, &(reply->addr_v4[i]), ip_[i], sizeof(ip_[i]));
    }
    
    sprintf(toWrite, "%s %s %s %s %s %d", ip_[0], ip_[1], ip_[2], ip_[3], ip_[4], sockfd);
    get_ip = true;
    LOGE("收到 %s", toWrite);
}

int manage() {
    static struct Msg msg;
    static ssize_t n;
    memset(&msg, 0, sizeof(struct Msg));
    n = 0;

    size_t needbs = sizeof(struct Msg_Hdr);
    int fd = sockfd;


    // 判断心跳包时间
    long t = time(NULL) - s;

    if(t > 60) {
        // 关闭套接字
        over_time = true;
        connected = false;
        // close(sockfd);
        LOGE("超时");
        return -1;
    }


    n = read(fd, &msg, needbs);
    if(n < 0) {
        fprintf(stderr, "read sockfd %d error: %s \n",fd,strerror(errno));
        Close(fd);
        return -1;
    }
    else if(n == 0) {
        fprintf(stderr, "recv 0 byte from server, close sockfd %d \n",fd);
        Close(fd);
        return -1;
    }
    else if(n == needbs){
        process_payload:
        char* ipv4_payload = msg.data;

        if(msg.hdr.type != 100 && msg.hdr.type != 104) {
            n = read(fd, ipv4_payload, msg.hdr.length);
            if(n != msg.hdr.length) {
                fprintf(stderr, "read payload error, need %d byte, read %d byte\n",msg.hdr.length, n);
                if(n <= 0) {
                    Close(fd);
                    // connected = false;
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
                LOGE("manage all");
                break;
            case 104:
                // 心跳包,记录接收时间
                s = time(NULL);
                LOGE("%s %ld", "收到心跳包104", s);
                break;
            default:
                LOGE("get %d", msg.hdr.type);
                // fprintf(stderr, "recv unknown type %d\n",msg.hdr.type);
                return -1;
        }
    }
    else {// 读到长度小于头长度说明可能出错(也有可能粘包,继续读取)
        while (n < needbs)
            n += read(fd, ((char*)&msg) + n , needbs-n);
        goto process_payload;
    }
    return 0;
}


void * new_th(void * arg) {
    while(connected) {
        if(manage() < 0 || !connected) {
            fprintf(stderr,"android back over\n");
            break;
        }
    }
}



/*
* 线程，处理收到的数据
*/
void * manage_data(void *arg) {
    int length, received_length, pack_type;
    char ip[20], router[20], dns1[20], dns2[20], dns3[20];
    char tun_buffer[MAXBUF + 1];
    char recv_buffer[MAXBUF + 1];
    struct Message send_msg, recv_msg;
    unsigned long int tid_tun = -1;

    static struct Msg msg;

    // 接收数据包的处理函数，应该采用线程方式
    while(connected) {
        // 读取虚接口信息，写入管道

        // LOGE("get");

        // 判断心跳包时间
        long t = time(NULL) - s;

        if(t > 60) {
            // 关闭套接字
            over_time = true;
            connected = false;
            // close(sockfd);
            LOGE("超时");
            return NULL;
        }

        memset(recv_buffer, 0, sizeof(recv_buffer));
        length = recv(sockfd, recv_buffer, MAXBUF, 0); // no &recv_buffer
        if(length < 0) {
            LOGE("error %s", strerror(errno));
            continue;
        }
        //LOGI("Receive Length: %d", length);
        memcpy(&recv_msg, recv_buffer, sizeof(recv_msg));

        LOGE("? %d", recv_msg.type);

        switch (recv_msg.type) {
            case 101:
                // IP响应
                sscanf(recv_msg.data, "%s%s%s%s%s", ip, router, dns1, dns2, dns3);
                sprintf(toWrite, "%s %s %s %s %s %d", ip, router, dns1, dns2, dns3, sockfd);
                get_ip = true;
                LOGE("orz %s", toWrite);
                break;
            case 103:
                // 上网应答
                // printf("上网应答：%d, 内容 %s\n", recv_msg.type, recv_msg.data);
                LOGE("%s", "收到103");
                // return NULL;
                continue;
                break;
            case 104:
                // 心跳包,记录接收时间
                LOGE("%s %ld", "收到104", s);
                s = time(NULL);
                // printf("%d 收到心跳包 %ld\n", recv_msg.type, s);
                LOGE("%s %ld", "收到104", s);
                break;
            
            default:
                LOGE("其他 %d %s\n", recv_msg.type, recv_msg.data);
                break;
               
        }


        // 间隔一秒
        sleep(1);
    }
}


int kill_myself() {
    LOGE("我要自杀");
    connected = false;
    if(sockfd) {
        close(sockfd);
    }
    return 0;
}
extern "C"

JNIEXPORT jstring JNICALL
Java_com_example_ipv4_1over_1ipv6_MainActivity_get_1info(JNIEnv *env, jobject instance) {

    // TODO


    return env->NewStringUTF(get_log());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_get_1info(JNIEnv *env, jobject instance) {

    // TODO

    // get_log();

    return env->NewStringUTF(get_log());
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MainActivity_startVpn(JNIEnv *env, jobject instance) {

    // TODO
    __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "MainActivity startvpn in c");

    int length;
    struct sockaddr_in6 server, client;
    char buffer[MAXBUF + 1];
    struct Message send_msg, recv_msg;
    memset(&recv_msg, 0, sizeof(recv_msg));

    // VPN链接时间
    long program_start_time_sec = time(NULL);

    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        // printf("资源分配失败\n");
        LOGE("can't create socket");
    }

    server.sin6_family = AF_INET6;
    client.sin6_family = AF_INET6;
    client.sin6_port = htons(VPN_SERVER_TCP_PORT_TEST);
    Inet_pton(AF_INET6, "2402:f000:2:c001:a453:97f6:b2af:90d3", &client.sin6_addr);
    if(test) {
        server.sin6_port = htons(VPN_SERVER_TCP_PORT_TEST);
        Inet_pton(AF_INET6, VPN_SERVER_IPV6_ADDRESS_TEST, &server.sin6_addr);
    }
    else {
        server.sin6_port = htons(VPN_SERVER_TCP_PORT);
        inet_pton(AF_INET6, VPN_SERVER_IPV6_ADDRESS, &server.sin6_addr);
    }

    int on = 1;
    SetSocket(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    Bind_Socket(sockfd, (struct sockaddr *)&client, sizeof(struct sockaddr_in6));


    int temp;
    if ((temp  = connect(sockfd, (struct sockaddr *) &server, sizeof(server)) )== -1){
        // fprintf(stderr, "can't access server\n" );
        __android_log_print(ANDROID_LOG_ERROR, TAG, "can't access server %s", strerror(errno));
        return -1;
    }

    // TODO 注释掉
    connected = true;

    LOGE("start");

    s = time(NULL);


    request_ipv4();

    LOGE("收到 request_ipv4");



    pthread_t recv_data;
    pthread_t heart_th;
    pthread_t new_data;
    // pthread_t web_th;

    int ret = 0;


    /*int ret = pthread_create(&recv_data, NULL, manage_data, NULL);
    if(ret) {
        LOGE("Create pthread error!");
        return 1;
    }
    */
    ret = pthread_create(&new_data, NULL, new_th, NULL);
    if(ret) {
        LOGE("Create pthread error!");
        return 1;
    }

    ret = pthread_create(&heart_th, NULL, send_heart, NULL);
    if(ret) {
        LOGE("Create pthread error!");
        return 1;
    }


    // pthread_create(&web_th, NULL, send_web_request, NULL);
    LOGE("wait thread");
    // pthread_join(recv_data, NULL);
    pthread_join(heart_th, NULL);
    pthread_join(new_data, NULL);
    // pthread_join(web_th, NULL);


    // manage_data(NULL);


    // close(sockfd);
    kill_myself();
    LOGE("Start Vpn END");


    return 0;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_kill(JNIEnv *env, jobject instance) {

    return kill_myself();

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_send_1fd(JNIEnv *env, jobject instance, jint fd, jstring net_file) {

    // 得到管道描述符
    tun_des = fd;
    const char *data = env->GetStringUTFChars(net_file, 0);
    PIPE_DIR = (char *) data;

    // 创建管道文件
    if (access(PIPE_DIR, F_OK) == -1)
    {
        mknod(PIPE_DIR, S_IFIFO | 0666, 0);

    }

    LOGE("文件创建成功 %s", PIPE_DIR);

    // 至此创建完成
    connected = true;
    time(&start_time);

    // 开启线程，读写虚接口
    pthread_t th_readTun;
    int ret = pthread_create(&th_readTun, NULL, readTun, NULL);
    
    if(ret) {
        LOGE("Create pthread error!");
        return 1;
    }
    return 0;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_send_1web_1requestt(JNIEnv *env, jobject instance,
                                                                   jstring data_, jint length) {
    const char *data = env->GetStringUTFChars(data_, 0);

    // TODO
    struct Message send_msg;
    __android_log_print(ANDROID_LOG_ERROR, TAG, "%d %lu %s", length, sizeof(*data), data) ;

    createMessage(&send_msg, WEB_REQUEST, (char *) data, length);

    LOGE("before");

    LOGE("type %d , length %d, data %s", send_msg.type, send_msg.length, send_msg.data);
    // 向服务器发送数据
    /*if(test) {
        if (send(sockfd, &send_msg, sizeof(struct Msg_Hdr) + send_msg.length, 0) < 0){
            // close(sockfd);
            __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", "发送失败") ;
            return -1;
        }
    }
    else {*/
        if (send(sockfd, &send_msg, sizeof(send_msg), 0) < 0){
            // close(sockfd);
            __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", "发送失败") ;
            return -1;
        }
    //}


    __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", "发送成功") ;

    env->ReleaseStringUTFChars(data_, data);

    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_send_1web_1request(JNIEnv *env, jobject instance,
                                                                  jcharArray data_, jint length) {
    jchar *data = env->GetCharArrayElements(data_, NULL);

    // TODO
    struct Message send_msg;
    __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s %d", (char *)data, length) ;

    createMessage(&send_msg, WEB_REQUEST, (char *) data, length);
    // 向服务器发送数据
    /*if(test) {
        if (send(sockfd, &send_msg, sizeof(struct Msg_Hdr) + send_msg.length, 0) < 0){
            // close(sockfd);
            __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", "发送失败") ;
            return -1;
        }
    }
    else {*/
        if (send(sockfd, &send_msg, sizeof(send_msg), 0) < 0){
            // close(sockfd);
            __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", "发送失败") ;
            return -1;
        }
    //}
    

    __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", "发送成功") ;


    env->ReleaseCharArrayElements(data_, data, 0);
    return 0;
}

/*
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_startVpn(JNIEnv *env, jobject instance) {

    __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "MyVpnService startvpn in c");

    int length;
    struct sockaddr_in6 server, client;
    char buffer[MAXBUF + 1];
    struct Message send_msg, recv_msg;
    memset(&recv_msg, 0, sizeof(recv_msg));

    // VPN链接时间
    long program_start_time_sec = time(NULL);

    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        // printf("资源分配失败\n");
        LOGE("can't create socket");
        return 0;
    }

    server.sin6_family = AF_INET6;
    client.sin6_family = AF_INET6;
    client.sin6_port = htons(VPN_SERVER_TCP_PORT_TEST);
    Inet_pton(AF_INET6, "2402:f000:2:c001:a453:97f6:b2af:90d3", &client.sin6_addr);
    if(test) {
        server.sin6_port = htons(VPN_SERVER_TCP_PORT_TEST);
        Inet_pton(AF_INET6, VPN_SERVER_IPV6_ADDRESS_TEST, &server.sin6_addr);
    }
    else {
        server.sin6_port = htons(VPN_SERVER_TCP_PORT);
        inet_pton(AF_INET6, VPN_SERVER_IPV6_ADDRESS, &server.sin6_addr);
    }

    int on = 1;
    SetSocket(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    Bind_Socket(sockfd, (struct sockaddr *)&client, sizeof(struct sockaddr_in6));


    int temp;
    if ((temp  = connect(sockfd, (struct sockaddr *) &server, sizeof(server)) )== -1){
        // fprintf(stderr, "can't access server\n" );
        __android_log_print(ANDROID_LOG_ERROR, TAG, "can't access server");
        return -1;
    }

    // connected = true;

    LOGE("start");

    s = time(NULL);


    request_ipv4();

    LOGE("after request_ipv4");



    pthread_t recv_data;
    pthread_t heart_th;
    // pthread_t web_th;


    int ret = pthread_create(&recv_data, NULL, manage_data, NULL);
    if(ret) {
        cout << "Create pthread error!" << endl;
        return 1;
    }

    ret = pthread_create(&heart_th, NULL, send_heart, NULL);
    if(ret) {
        cout << "Create pthread error!" << endl;
        return 1;
    }

    for(int i = 0;i < 3;i++) {
        cout <<  "This is the main process." << endl;
        sleep(1);
    }

    // pthread_create(&web_th, NULL, send_web_request, NULL);
    LOGE("wait thread");
    pthread_join(recv_data, NULL);
    pthread_join(heart_th, NULL);
    // pthread_join(web_th, NULL);


    // manage_data(NULL);


    // close(sockfd);
    kill_myself();
    LOGE("END");


    return 0;

}
*/

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_isGet_1ip(JNIEnv *env, jobject instance) {

    // TODO
    // __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", toWrite) ;
    return (jboolean) get_ip;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_ip_1info(JNIEnv *env, jobject instance) {

    // TODO 获取ip信息
    return env->NewStringUTF(toWrite);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ipv4_1over_1ipv6_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject  obj) {

    /*
     *
     * 方法名称规定 : Java_完整包名类名_方法名()
     * JNIEnv 指针
     *
     * 参数介绍 :
     * env : 代表Java环境, 通过这个环境可以调用Java中的方法
     * this : 代表调用JNI方法的对象, 即MainActivity对象
     * */

    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}




