#include <brpc/closure_guard.h>
#include <brpc/controller.h>
#include <brpc/server.h>
#include <butil/logging.h>
#include <cstdint>
#include <cstdlib>
#include <google/protobuf/service.h>
#include <memory>
#include <sys/stat.h>

#include "base.pb.h"
#include "file.pb.h"

#include "utils.h"
#include "etcd.h"
#include "log.hpp"

namespace im
{
class FileServiceImpl : public FileService
{
  public:
    FileServiceImpl(const std::string &storage_path) : _storage_path(storage_path)
    {
        umask(0);
        mkdir(storage_path.c_str(), 0775);
        if (_storage_path.back() != '/')
            _storage_path += '/';
    }
    ~FileServiceImpl() override = default;

    void GetSingleFile(::google::protobuf::RpcController *controller, const ::im::GetSingleFileRequest *request,
                       ::im::GetSingleFileResponse *response, ::google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard done_guard(done);
        response->set_request_id(request->request_id());
        std::string fid = request->file_id();
        std::string filename = _storage_path + fid;

        std::string body;
        bool ret = ReadFile(filename, body);
        if (!ret)
        {
            response->set_success(false);
            response->set_error("读取文件失败");
            LOG_ERROR("{} 读取文件数据失败!", request->request_id());
            return;
        }
        
        response->set_success(true);
        response->mutable_file_data()->set_file_id(fid);
        response->mutable_file_data()->set_file_content(body);
    }

    void GetMultiFile(::google::protobuf::RpcController *controller, const ::im::GetMultiFileRequest *request,
                      ::im::GetMultiFileResponse *response, ::google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard done_guard(done);
        response->set_request_id(request->request_id());
       for (int i = 0; i < request->file_id_list_size(); ++i) 
       {
            std::string fid = request->file_id_list(i);
            std::string filename = _storage_path + fid;
            std::string body;

            bool ret = ReadFile(filename, body);
            if (ret == false)
            {
                response->set_success(false);
                response->set_error("读取文件失败");
                LOG_ERROR("{} 读取文件数据失败!", request->request_id());
                return;
            }
            FileDownloadData data;
            data.set_file_id(fid);
            data.set_file_content(body);
            response->mutable_file_data()->emplace(fid, data);       
        }
        response->set_success(true);
    }

    void PutSingleFile(::google::protobuf::RpcController *controller, const ::im::PutSingleFileRequest *request,
                       ::im::PutSingleFileResponse *response, ::google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard done_guard(done);
        response->set_request_id(request->request_id());
        std::string fid = Uuid();
        std::string filename = _storage_path + fid;
        bool ret = WriteFile(filename, request->file_data().file_content());
        if (ret == false)
        {
            response->set_success(false);
            response->set_error("写入文件失败");
            LOG_ERROR("{} 写入文件数据失败!", request->request_id());
            return;
        }

        response->set_success(true);
        response->mutable_file_info()->set_file_id(fid);
        response->mutable_file_info()->set_file_name(request->file_data().file_name());
        response->mutable_file_info()->set_file_size(request->file_data().file_content().size());
    }

    void PutMultiFile(::google::protobuf::RpcController *controller, const ::im::PutMultiFileRequest *request,
                      ::im::PutMultiFileResponse *response, ::google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard done_guard(done);
        response->set_request_id(request->request_id());

        for (int i = 0; i < request->file_data_size(); ++i) 
        {
            const FileUploadData &data = request->file_data(i);
            std::string fid = Uuid();
            std::string filename = _storage_path + fid;
            bool ret = WriteFile(filename, data.file_content());
            if (ret == false)
            {
                response->set_success(false);
                response->set_error("写入文件失败");
                LOG_ERROR("{} 写入文件数据失败!", request->request_id());
                return;
            }
            FileMessageInfo *file_info = response->add_file_info();
            file_info->set_file_id(fid);
            file_info->set_file_name(data.file_name());
            file_info->set_file_size(data.file_content().size());
        }
    }

  private:
    std::string _storage_path;
};

class FileServer 
{
public:
    using Ptr = std::shared_ptr<FileServer>;

    FileServer(const Register::Ptr &reg_client, 
               const std::shared_ptr<brpc::Server> &rpc_server)
        : _reg_client(reg_client), _rpc_server(rpc_server)
    {}

    void Start() 
    {
        _rpc_server->RunUntilAskedToQuit();
    }

private:
    Register::Ptr _reg_client;
    std::shared_ptr<brpc::Server> _rpc_server;
};

class FileServerBuilder
{
public:
    void MakeRegObject(const std::string& reg_host, 
        const std::string& service_name,
        const std::string& access_host)
    {
        _reg_client = std::make_shared<Register>(reg_host);
        _reg_client->Registry(service_name, access_host);
    }

    void MakePrcServer(uint16_t port, int32_t timeout,
            uint8_t num_threads, const std::string& path = "./data/")
    {
        _rpc_server = std::make_shared<brpc::Server>();
        FileServiceImpl* file_service = new FileServiceImpl(path);
        int ret = _rpc_server->AddService(file_service, 
            brpc::SERVER_OWNS_SERVICE);
        if (ret == -1)
        {
            LOG_ERROR("添加RPC服务失败!");
            abort();
        }
        brpc::ServerOptions options;
        options.idle_timeout_sec = timeout;
        options.num_threads = num_threads;
        if (_rpc_server->Start(port, &options) != 0)
        {
            LOG_ERROR("启动RPC服务失败!");
            abort();
        }
    }

    FileServer::Ptr Build()
    {
        if (!_reg_client)
        {
            LOG_ERROR("还初始化服务注册模块！");
            abort();
        }

        if (!_rpc_server)
        {
            LOG_ERROR("还初始化RPC服务模块!");
            abort();
        }
        FileServer::Ptr server = std::make_shared<FileServer>(_reg_client, _rpc_server);
        return server;
    }

private:
    Register::Ptr _reg_client;
    std::shared_ptr<brpc::Server> _rpc_server;
};


}; // namespace im