
#include "unp.h"
extern "C"

using namespace std;



#define big_size 2014
#define small_size 50
#define MAXBUF 10240
#define MAX_FIFO_BUF 1024
#define MAX_MESSAGE_LENGTH 4096
#define MAX_PACKET_LENGTH 65536


#define VPN_SERVER_IPV6_ADDRESS "2402:f000:1:4417::900"
#define VPN_SERVER_TCP_PORT 5678

// #define VPN_SERVER_IPV6_ADDRESS_TEST "2402:f000:5:8601:942c:3463:c810:6147"
#define VPN_SERVER_IPV6_ADDRESS_TEST "2402:f000:5:8601:8e23:e6e3:b7:3110"
#define VPN_SERVER_TCP_PORT_TEST 6666

#define KB 1024
#define MB 1048576
#define GB 1073741824

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
char ip_log[MAX_MESSAGE_LENGTH];
int byte_in = 0;
int byte_out = 0;
int total_byte = 0;
int packet_in = 0;
int packet_out = 0;
int total_packet = 0;
int byte_in_last = 0;
int byte_out_last = 0;
long last_time = 0;
bool kill_it = false;

// 连接信息
char server_ip[MAX_MESSAGE_LENGTH];
char ipv4[MAX_MESSAGE_LENGTH];
char out_flow[MAX_MESSAGE_LENGTH];
char in_flow[MAX_MESSAGE_LENGTH];
char total_flow[MAX_MESSAGE_LENGTH];
char v6[MAX_MESSAGE_LENGTH];
char up_speed[MAX_MESSAGE_LENGTH];
char down_speed[MAX_MESSAGE_LENGTH];

bool server_ready = false;

char *SERVER_IPV6;
int SERVER_PORT = 6666;


void num_to_MGB(int BYTE, char* data_flow) {
    // char data_flow[MAX_MESSAGE_LENGTH];
    if(BYTE < KB) {
        sprintf(data_flow, "%d B", BYTE);
    } else if(BYTE < MB) {
        sprintf(data_flow, "%.2f KB", float(BYTE)/float(KB));
    } else if(BYTE < GB){
        sprintf(data_flow, "%.2f MB", float(BYTE)/float(MB));
    } else {
        sprintf(data_flow, "%.2f GB", float(BYTE)/float(GB));
    }
    // return data_flow;
}


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

int kill_myself() {
    LOGE("Kill Back C");
    connected = false;

    byte_in = 0;
    byte_out = 0;
    total_byte = 0;
    packet_in = 0;
    packet_out = 0;
    total_packet = 0;
    // run_time = 0;
    last_time = 0;
    byte_in_last = 0;
    byte_out_last = 0;
    server_ready = false;
    get_ip = false;
    if(sockfd) {
        Close(sockfd);
    }
    LOGE("kill over");
    return 0;
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

}


void * send_heart(void *arg) {
    struct Msg send_msg;

    send_msg.hdr.length = 0;
    send_msg.hdr.type = HEART_BEAT;
    memset(&(send_msg.data), 0, sizeof(send_msg.data));
    // 创建发送数据包
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

        long used = run_time - last_time;
        if(used < 1) {
            used = 1;
        }
        last_time = run_time;
        run_info = gmtime(&t);
        // VPN开启时间
        sprintf(packet_log, "%s", ip_log);
        // sprintf(packet_log, "已运行:%02d:%02d:%02d %ld %ld %ld\n", run_info->tm_hour, run_info->tm_min, run_info->tm_sec, t, start_time, run_time);
        sprintf(packet_log, "%s已运行: %02d:%02d:%02d\n", packet_log,  run_info->tm_hour, run_info->tm_min, run_info->tm_sec);

        num_to_MGB(byte_out, out_flow);
        num_to_MGB(byte_in, in_flow);
        num_to_MGB(total_byte, total_flow);
        // sprintf(packet_log, "%s已发送: %d B/%d 个数据包\n已接收: %dB/%d个数据包\n共出入: %dB/%d个数据包", packet_log, byte_out, packet_out, byte_in, packet_in, total_byte, total_packet );
        sprintf(packet_log, "%s已发送: %s/%d 个数据包\n已接收: %s/%d个数据包\n共出入: %s/%d个数据包\n", packet_log, out_flow, packet_out, in_flow, packet_in, total_flow, total_packet );
        LOGE("时间%ld", used);
        sprintf(packet_log, "%s上传:  %.2f KB/s\n下载: %.2f KB/s\n", packet_log, float(byte_out - byte_out_last)/float(used * 1024), float(byte_in - byte_in_last)/float(used * 1024));
        sprintf(packet_log, "%s上联V6: %s\n", packet_log, v6);
        byte_out_last = byte_out;
        byte_in_last = byte_in;
    }
    else {
        sprintf(packet_log, "Top Vpn is connecting....");
    }

    // LOGE("%s", packet_log);
    return packet_log;
}

void * readTun(void *arg) {
    // 读取虚接口的线程，while循环反复读取

    struct Msg send_msg;
    // 虚接口读出的数据包
    char packet[MAX_PACKET_LENGTH];

    while(connected) {
        // 持续读取虚接口

        // LOGE("readTun");

        memset(&send_msg, 0, sizeof(send_msg));
        memset(packet, 0, sizeof(packet));

        int length = read(tun_des, packet, sizeof(packet));

        // LOGE("after read tun %d", length);
        if(length > 0) {
            // 封装数据
            // LOGE("read from tunnel %d %s", length, packet);
            // debugPacket(packet);

            send_msg.hdr.type = WEB_REQUEST;
            send_msg.hdr.length = length;
            memcpy(send_msg.data, packet, length);
            // 发送IPV6数据包
            int temp = write(sockfd, (char*)&send_msg, length + sizeof(struct Msg_Hdr));
            if(temp <= 0) {
                LOGE("102 发送失败");
                continue;
            }

            // 记录读取的长度和次数
            debugPacket(&send_msg, send_msg.hdr.length);
            byte_out += length + 5;
            packet_out += 1;
            total_byte += length + 5;
            total_packet += 1;
            LOGE("102 发送成功 %d msg.length: %d write_length:%d %ld %ld %s", send_msg.hdr.type, send_msg.hdr.length, temp, sizeof(struct Msg_Hdr), length + sizeof(struct Msg_Hdr), send_msg.data);


            // get_log();
        }
    }
    LOGE("after read Tun");
    return NULL;
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
    Write_nByte(tun_des, payload, len);
    return 0;
}


void recv_ipv4_packet(Msg* msg) {
    write_tun(msg->data, msg->hdr.length);
    debugPacket_recv(msg, msg->hdr.length);
}

void recv_ipv4_addr(Msg* msg) {
    Ipv4_Request_Reply* reply = (Ipv4_Request_Reply*)msg->data;
    for(int i = 0 ; i < 5; ++i) {
        inet_ntop(AF_INET, &(reply->addr_v4[i]), ip_[i], sizeof(ip_[i]));
    }
    sprintf(ip_log, "IPV4:    \t%s\nRouter:\t%s\nDNS:    \t%s %s %s\n", ip_[0], ip_[1], ip_[2], ip_[3], ip_[4]);
    sprintf(toWrite, "%s %s %s %s %s %d", ip_[0], ip_[1], ip_[2], ip_[3], ip_[4], sockfd);
    get_ip = true;
    LOGE("收到 %s", toWrite);
}

int manage() {
    static struct Msg msg;
    static ssize_t n;
    memset(&msg, 0, sizeof(struct Msg));
    n = 0;

    int fd = sockfd;


    // 判断心跳包时间
    long t = time(NULL) - s;

    if(t > 60) {
        // 关闭套接字
        over_time = true;
        // connected = false;
        // close(sockfd);
        LOGE("超时 kill");
        kill_myself();
        LOGE("超时");
        return -1;
    }

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
    return 0;
}


void * new_th(void * arg) {
    while(connected) {
        if(manage() < 0 || !connected) {
            fprintf(stderr,"android back over\n");
            break;
        }
    }
    return NULL;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MainActivity_kill(JNIEnv *env, jobject instance) {

    // TODO
    LOGE("Main activity call kill");
    kill_myself();
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_send_1addr_1port(JNIEnv *env, jobject instance,
                                                                jstring addr_, jint port) {
    const char *addr = env->GetStringUTFChars(addr_, 0);

    // TODO

    SERVER_IPV6 = (char *) addr;
    LOGE("before cp");
    memcpy(v6, (char *)addr, strlen(addr));
    LOGE("after cp %s", v6);
    SERVER_PORT = port;

    env->ReleaseStringUTFChars(addr_, addr);

    server_ready = true;

    return 0;
}

extern "C"

JNIEXPORT jstring JNICALL
Java_com_example_ipv4_1over_1ipv6_MainActivity_get_1info(JNIEnv *env, jobject instance) {

    // TODO
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

    while (!server_ready) {
        // 等待前端发送服务器ip
    }

    server.sin6_family = AF_INET6;
    client.sin6_family = AF_INET6;
    client.sin6_port = htons(VPN_SERVER_TCP_PORT_TEST);
    Inet_pton(AF_INET6, "2402:f000:2:c001:a453:97f6:b2af:90d3", &client.sin6_addr);
    server.sin6_port = htons(SERVER_PORT);

    Inet_pton(AF_INET6, SERVER_IPV6, &server.sin6_addr);

    int on = 1;
    SetSocket(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    Bind_Socket(sockfd, (struct sockaddr *)&client, sizeof(struct sockaddr_in6));

    int temp;
    if ((temp  = connect(sockfd, (struct sockaddr *) &server, sizeof(server)) )== -1){
        // fprintf(stderr, "can't access server\n" );
        __android_log_print(ANDROID_LOG_ERROR, TAG, "can't access server %s", strerror(errno));
        return -1;
    }

    LOGE("after connect");

    // TODO 注释掉
    connected = true;

    LOGE("start");

    s = time(NULL);


    request_ipv4();
    LOGE("收到 request_ipv4");

    // 两个线程分别用于处理数据包和发送心跳包
    pthread_t recv_data;
    pthread_t heart_th;
    int ret = 0;
    ret = pthread_create(&recv_data, NULL, new_th, NULL);
    if(ret) {
        LOGE("Create pthread error!");
        return 1;
    }
    ret = pthread_create(&heart_th, NULL, send_heart, NULL);
    if(ret) {
        LOGE("Create pthread error!");
        return 1;
    }
    pthread_join(heart_th, NULL);
    pthread_join(recv_data, NULL);
    LOGE("startVpn ready to kill");
    kill_myself();
    LOGE("Start Vpn END");


    return 0;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_kill(JNIEnv *env, jobject instance) {

    LOGE("Vpnservice java call kill");
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
JNIEXPORT jboolean JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_isGet_1ip(JNIEnv *env, jobject instance) {

    // TODO
    // 检查是否已经获得IP
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




