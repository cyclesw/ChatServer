//
// Created by 19396 on 25-8-3.
//

#ifndef SPEECH_SERVER_H
#define SPEECH_SERVER_H

#include <brpc/server.h>
#include <cstdint>
#include <cstdlib>
#include <memory>

#include "asr.h"
#include "etcd.h"
#include "log.hpp"

#include "speech.pb.h"


namespace im
{

class SpeechServiceImpl : public im::SpeechService
{
  public:
    explicit SpeechServiceImpl(const ASRClient::Ptr &asr_client) : _asr_client(asr_client)
    {
    }

    ~SpeechServiceImpl() override = default;

    void SpeechRecognition(google::protobuf::RpcController *controller, const im::SpeechRecognitionRequest *request,
                           im::SpeechRecognitionResponse *response, google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard rpc_guard(done);
        LOG_TRACE("SpeechRecognition request: {}", request->DebugString());
        std::string message;

        bool ret = _asr_client->Recognize(request->speech_content(), message);
        response->set_request_id(request->request_id());

        if (!ret)
        {
            LOG_ERROR("{} 语音识别失败", request->request_id());
            response->set_success(false);
            response->set_error(message);
            return;
        }

        response->set_success(true);
        response->set_recognition_result(message);
    }

  private:
    ASRClient::Ptr _asr_client;
};

class SpeechServer
{
public:
    using Ptr = std::shared_ptr<SpeechServer>;
    SpeechServer(const ASRClient::Ptr &asr_client, 
                 const Register::Ptr &reg_client, 
                 const std::shared_ptr<brpc::Server> &rpc_server)
        : _asr_client(asr_client), _reg_client(reg_client), _rpc_server(rpc_server)
    {
        LOG_INFO("语音识别服务器初始化成功");
    }
    
    /*!
     * @brief 搭建RPC服务器，并启动服务器
     */
    void Start()
    {
        _rpc_server->RunUntilAskedToQuit();
    }

private:
    ASRClient::Ptr _asr_client;
    Register::Ptr _reg_client;
    std::shared_ptr<brpc::Server> _rpc_server;
};

class SpeechServerBuilder
{
public:
    /*!
     * @brief 构造语音识别客户端对象
     */
    void MakeAsrObject(const std::string& app_id, const std::string& api_key, const std::string& secret_key)
    {
        _asr_client = std::make_shared<ASRClient>(app_id, api_key, secret_key);
    }


    /*!
     * @brief 构造服务注册服务器客户端对象
     */
    void MakeRegObject(const std::string& reg_host, 
        const std::string&  service_name,
        const std::string& access_host)
    {
        _reg_client = std::make_shared<Register>(reg_host);
        _reg_client->Registry(service_name, access_host);
    }

    /*!
     * @brief 构造语音识别服务器对象
     */    
    void MakeRpcServer(uint16_t port, int32_t timeout, uint8_t num_threads)
    {
        if (!_asr_client)
        {
            LOG_ERROR("还未初始化语音识别模块");
            abort();
        }
        
        _rpc_server = std::make_shared<brpc::Server>();
        SpeechServiceImpl* speech_service = new SpeechServiceImpl(_asr_client);

        int ret = _rpc_server->AddService(speech_service, brpc::SERVER_DOESNT_OWN_SERVICE);
        if (ret != 0)
        {
            LOG_ERROR("添加RPC服务失败");
            abort();
        }
        brpc::ServerOptions options;
        options.idle_timeout_sec = timeout;
        options.num_threads = num_threads;
        ret = _rpc_server->Start(port, &options);
        if (ret != 0)
        {
            LOG_ERROR("启动RPC服务器失败");
            abort();
        }
    }

    SpeechServer::Ptr Build()
    {
        if (!_asr_client)
        {
            LOG_ERROR("还为初始化语音识别模块");
            abort();
        }

        if (!_reg_client)
        {
            LOG_ERROR("还未进行服务注册模块");
            abort();
        }

        if(!_rpc_server)
        {
            LOG_ERROR("还未初始化RPC服务器模块");
            abort();
        }

        SpeechServer::Ptr server = std::make_shared<SpeechServer>(
            _asr_client, _reg_client, _rpc_server);
        
        return server;
    }

private:
    ASRClient::Ptr _asr_client;
    Register::Ptr _reg_client;
    std::shared_ptr<brpc::Server> _rpc_server;
};

} // namespace im

#endif // SPEECH_SERVER_H
