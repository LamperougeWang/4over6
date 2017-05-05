
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

#define VPN_SERVER_IPV6_ADDRESS_TEST "2402:f000:5:8601:8e23:e6e3:b7:3110"

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
    char ipv4_payload[MAX_MESSAGE_LENGTH];
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
// 链接服务器的socket
int sockfd;
// 是否超时
bool over_time = false;
// 存活标记
int live_flag = 0;
//time_t是long类型，精确到秒，是当前时间和1970年1月1日零点时间的差
time_t s = time(NULL);

bool test = true;

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
        close(sockfd);
        // fprintf(stderr, "发送失败\n");
        LOGE("IPv4请求失败");
    }

    LOGE("%s", "IPV4地址请求");

    struct Msg msg;

    if(test) {
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

    }

}


void * send_heart(void *arg) {
    struct Message send_msg;

    send_msg.length = 0;
    send_msg.type = HEART_BEAT;
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
        if (send(sockfd, &send_msg, sizeof(send_msg), 0) < 0)
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
        //LOGI("Receive Length: %d", length);
        memcpy(&recv_msg, recv_buffer, sizeof(recv_msg));

        LOGE("? %d", recv_msg.type);

        switch (recv_msg.type) {
            case 101:
                // IP响应
                sscanf(recv_msg.data, "%s%s%s%s%s", ip, router, dns1, dns2, dns3);
                sprintf(toWrite, "%s %s %s %s %s %d", ip, router, dns1, dns2, dns3, sockfd);
                get_ip = true;
                LOGE("%s", toWrite);
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

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_startVpn(JNIEnv *env, jobject instance) {

    __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "Your params is null");

    // TODO
    LOGE("c");


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
    Inet_pton(AF_INET6, "2402:f000:2:c001:714e:1135:b3ed:95f5", &client.sin6_addr);
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

    connected = true;

    LOGE("start");

    s = time(NULL);


    request_ipv4();



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

    pthread_join(recv_data, NULL);
    pthread_join(heart_th, NULL);
    // pthread_join(web_th, NULL);


    // manage_data(NULL);


    LOGE("END");


    return 0;

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_isGet_1ip(JNIEnv *env, jobject instance) {

    // TODO
    __android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "%s", toWrite) ;
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




