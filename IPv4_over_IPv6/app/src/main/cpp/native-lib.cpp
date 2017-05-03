#include <jni.h>
#include <string>

#include <android/log.h>
#include <Android/log.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include   <stdint.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include    <netinet/in.h>

#include <netinet/ether.h>
#include    <netinet/ip.h>
#include    <netinet/ip6.h>
#include    <netinet/tcp.h>
#include    <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include <linux/netlink.h>
using namespace std;

#define TAG "Back JNI" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型

static int  tun_fd;
int pipe_write_fd, pipe_read_fd;

int flag; //
#define MAX_IPV4_PAYLOAD 1024
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

struct Msg_Hdr {
    uint32_t length; // payload 长度,不包括type, 注意协议切割
    char type; //
};
struct Msg{
    struct Msg_Hdr hdr;
    char ipv4_payload[MAX_IPV4_PAYLOAD];
};

void request_ipv4(int fd);
void connect_server();
void recv_ipv4_addr(Msg* msg);
void do_keep_alive();
void recv_ipv4_packet(Msg* msg);



JNIEXPORT jint JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_startVpn(JNIEnv *env, jobject instance) {

    // TODO



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




