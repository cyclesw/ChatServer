#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <cstdint>
#include <string>
#include <vector>

class MyTcpHandler : public AMQP::TcpHandler
{
public:
    /**
     * @brief 创建新连接时回调
     * 
     * @param connection 附加到处理程序的连接
     */
    void onAttached(AMQP::TcpConnection *connection) override
    {

    }

    /**
     * @brief TCP连接时回调
     * 
     * @param connection  现在可以使用的连接
     */
    void onConnected(AMQP::TcpConnection *connection) override
    {

    }

    /**
     * @brief 建立安全TLS连接时调用的方法
     * 
     * @param connection  已被保护的连接
     * @param ssl 来自openssl库的ssl结构
     */
    bool onSecured(AMQP::TcpConnection *connection, const SSL *ssl) override
    {

    }

    /**
     * @brief 登录尝试成功时由AMQP库调用的方法。
     * 
     * @param connection 
     */
    void onReady(AMQP::TcpConnection *connection) override
    {

    }

    /**
     * @brief 该方法在服务器尝试协商检测信号间隔时调用，
     * 
     * @param connection 发生错误的连接
     * @param interval  建议的间隔
     * @return uint16_t 
     */
    uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval) override
    {

    }

    /**
     * @brief 发生致命错误时由 AMQP 库调用的方法
     * 
     * @param connection 发生错误的连接
     * @param message 错误信息
     */
    void onError(AMQP::TcpConnection *connection, const char *message) override
    {

    }
    /**
     * @brief 该方法在 AMQP 协议结束时调用，
     * 
     * @param connection 
     */
    void onClosed(AMQP::TcpConnection *connection) override
    {

    }

    /**
     * @brief 连接关闭或丢失时调用的方法
     * 
     * @param connection 
     */
    void onLost(AMQP::TcpConnection *connection) override
    {

    }

    /**
     * @brief 调用的最终方法。这表示将不再对处理程序进行有关连接的进一步调
     * 
     */
     void onDetached(AMQP::TcpConnection *connection) override
     {

     }

     /**
      * @brief 当 AMQP-CPP 库想要与主事件循环交互时，它会调用该方法。
      * 
      * @param connection 想要与事件循环交互的连接
      * @param fd 应该检查的文件描述符
      * @param flags 标记位或AMQP::可读、AMQP::可写
      */
     void monitor(AMQP::TcpConnection *connection, int fd, int flags) override
     {
     }
};

int main()
{
    std::cout <<"123123" << std::endl;
    std::string s = "asd";
    std::vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);


    return 0;
}