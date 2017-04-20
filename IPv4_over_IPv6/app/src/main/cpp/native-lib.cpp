#include <jni.h>
#include <string>
using namespace std;

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ipv4_1over_1ipv6_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

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

#define MAX_MESSAGE_LENGTH 4096

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

JNIEXPORT jstring JNICALL
Java_com_example_ipv4_1over_1ipv6_MyVpnService_send(
        JNIEnv *env,
        jobject obj, jint type) {

    /*
     * 发送type型数据，并接收
     * */

    switch (type) {
        case WEB_REQUEST:
            printf("上网请求\n");
        default:
            printf("23");
    }



    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());


}


int send_request_ip(int server_socket) {

}