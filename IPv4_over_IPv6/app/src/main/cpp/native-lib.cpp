#include <jni.h>
#include <string>
#include<iostream>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <android/log.h>


using namespace std;

#define TAG "Back JNI" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型

#define big_size 2014
#define small_size 50
#define MAXBUF 10240
#define MAX_FIFO_BUF 1024
#define MAX_MESSAGE_LENGTH 4096
#define VPN_SERVER_IPV6_ADDRESS "2402:f000:1:4417::900"
#define VPN_SERVER_TCP_PORT 5678
// 客户端消息结构体
struct Message {
    int length;                             // 整个结构体的字节长度
    char type;                              // 类型
    char data[MAX_MESSAGE_LENGTH];          // 数据段
};

// 100 IP地址请求
const int IP_REQUEST = 100;
// 101 IP地址回应
const int IP_RESPONSE = 101;
// 102 上网请求
const int WEB_REQUEST = 102;
// 103 上网回应
const int WEB_RESPONSE = 103;
// 104 心跳包
const int HEART_BEAT = 104;

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


void request_ipv4(int sockfd) {
    struct Message send_msg;

    // 创建发送数据包
    send_msg.length = 5;
    send_msg.type = IP_REQUEST;
    memset(&(send_msg.data), 0, sizeof(send_msg.data));


    // 向服务器发送数据
    if (send(sockfd, &send_msg, sizeof(send_msg), 0) < 0)
    {
        close(sockfd);
        fprintf(stderr, "发送失败\n");
    }
}


void * send_heart(void *arg) {
    struct Message send_msg;

    // 创建发送数据包
    send_msg.length = 5;
    send_msg.type = HEART_BEAT;
    memset(&(send_msg.data), 0, sizeof(send_msg.data));
    printf("%lu\n", sizeof(send_msg));
    while(!over_time) {
        printf("发送心跳包%ld\n", time(NULL));

        // 向服务器发送数据
        if (send(sockfd, &send_msg, sizeof(send_msg), 0) < 0)
        {
            close(sockfd);
            fprintf(stderr, "发送失败\n");
        }

        sleep(20);
    }

    return NULL;
}


void * send_web_request(void *arg) {
    struct Message send_msg;

    // 创建发送数据包
    send_msg.length = 5;
    send_msg.type = WEB_REQUEST;
    char data[] = "2333sfhs";
    memset(&(send_msg.data), 0, sizeof(send_msg.data));
    memcpy(&(send_msg.data), data, sizeof(data));

    printf("%lu\n", sizeof(send_msg.data));
    printf("%lu\n", strlen(send_msg.data));

    while(!over_time) {
        printf("发送数据\n");

        // 向服务器发送数据
        if (send(sockfd, &send_msg, sizeof(send_msg), 0) < 0)
        {
            close(sockfd);
            fprintf(stderr, "发送失败\n");
        }

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



    // 接收数据包的处理函数，应该采用线程方式
    while(1) {
        // 读取虚接口信息，写入管道

        // 判断心跳包时间
        long t = time(NULL) - s;

        if(t > 60) {
            // 关闭套接字
            over_time = true;
            return NULL;
        }




        memset(recv_buffer, 0, sizeof(recv_buffer));
        length = recv(sockfd, recv_buffer, MAXBUF, 0); // no &recv_buffer
        //LOGI("Receive Length: %d", length);
        memcpy(&recv_msg, recv_buffer, sizeof(recv_msg));

        switch (recv_msg.type) {
            case IP_RESPONSE:
                // IP响应
                sscanf(recv_msg.data, "%s%s%s%s%s", ip, router, dns1, dns2, dns3);

                sprintf(toWrite, "%s %s %s %s %s %d", ip, router, dns1, dns2, dns3, sockfd);
                printf("收到IP回应%s\n", toWrite);
                get_ip = true;
                LOGD("收到101");
                LOGD(toWrite);
                break;
            case WEB_RESPONSE:
                // 上网应答
                printf("上网应答：%d, 内容 %s\n", recv_msg.type, recv_msg.data);
                LOGD("收到103");
                return NULL;
                break;
            case HEART_BEAT:
                // 心跳包,记录接收时间
                s = time(NULL);
                printf("%d 收到心跳包 %ld\n", recv_msg.type, s);
                LOGD("收到104");
                break;

            default:
                printf("其他 %d %s\n", recv_msg.type, recv_msg.data);
                break;
        }
        // 间隔一秒
        sleep(1);
    }
}

JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_startVpn(JNIEnv *env, jobject instance) {

    // TODO
    int length;
    struct sockaddr_in6 server;
    char buffer[MAXBUF + 1];
    struct Message send_msg, recv_msg;
    memset(&recv_msg, 0, sizeof(recv_msg));

    // VPN链接时间
    long program_start_time_sec = time(NULL);

    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        printf("资源分配失败\n");
    }

    server.sin6_family = AF_INET6;
    server.sin6_port = htons(VPN_SERVER_TCP_PORT);
    inet_pton(AF_INET6, VPN_SERVER_IPV6_ADDRESS, &server.sin6_addr);

    int temp;
    if ((temp  = connect(sockfd, (struct sockaddr *) &server, sizeof(server)) )== -1){
        fprintf(stderr, "can't access server\n" );
    }

    LOGD("start");

    s = time(NULL);


    request_ipv4(sockfd);

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


    return 0;

}

JNIEXPORT jboolean JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_isGet_1ip(JNIEnv *env, jobject instance) {

    // TODO
    return (jboolean) get_ip;
}

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




