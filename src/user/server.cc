#include "server.h"
#include "file.pb.h"
#include "utils.h"
#include "channel.h"
#include "user.hxx"
#include "database/mysql.hpp"

#include <brpc/channel.h>
#include <brpc/server.h>
#include <sw/redis++/redis.h>

#include "database/redis.h"

// TODO: 缺失了异常处理，待完善 (after code function complete)

using namespace im;

UserServiceImpl::UserServiceImpl(const DMSClient::Ptr &dm_client,
                                 const std::shared_ptr<elasticlient::Client> &es_client,
                                 const std::shared_ptr<odb::core::database> &mysql_client,
                                 const std::shared_ptr<sw::redis::Redis> &redis_client,
                                 const ServiceManager::Ptr &channel_manager, const std::string &file_service_name) :
    _es_user(std::make_shared<ESUser>(es_client)), _mysql_user(std::make_shared<UserTable>(mysql_client)),
    _redis_session(std::make_shared<Session>(redis_client)), _redis_status(std::make_shared<Status>(redis_client)),
    _redis_codes(std::make_shared<Codes>(redis_client)), _file_service_name(file_service_name),
    _channels(channel_manager), _dms_client(dm_client)
{
    _es_user->CreateIndex();
}

UserServiceImpl::~UserServiceImpl() = default;

bool UserServiceImpl::NicknameCheck(const std::string &nickname)
{
    return nickname.size() < 22;
}

bool UserServiceImpl::PasswordCheck(const std::string &password)
{
    if (password.size() < 6 || password.size() > 15)
    {
        LOG_ERROR("密码长度不合法：{}-{}", password, password.size());
        return false;
    }
    for (int i = 0; i < password.size(); i++)
    {
        if (!((password[i] > 'a' && password[i] < 'z') ||
              (password[i] > 'A' && password[i] < 'Z') ||
              (password[i] > '0' && password[i] < '9') ||
              password[i] == '_' || password[i] == '-'))
        {
            LOG_ERROR("密码字符不合法：{}", password);
            return false;
        }
    }
    return true;
}

bool UserServiceImpl::PhoneCheck(const std::string &phone)
{
    if (phone.size() != 11)
        return false;
    if (phone[0] != '1')
        return false;
    if (phone[1] < '3' || phone[1] > '9')
        return false;
    for (int i = 2; i < 11; i++)
    {
        if (phone[i] < '0' || phone[i] > '9')
            return false;
    }
    return true;
}

void UserServiceImpl::UserRegister(google::protobuf::RpcController *controller, const im::UserRegisterRequest *request,
                                   im::UserRegisterResponse *response, google::protobuf::Closure *done)
{
    LOG_TRACE("收到用户注册请求");
    brpc::ClosureGuard rpc_guard(done);

    auto err_response = [&response](const std::string &rid,
                                    const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };

    std::string nickname = request->nickname();
    std::string password = request->password();
    bool ret = NicknameCheck(nickname);
    if (!ret)
    {
        LOG_TRACE("{} - 用户名长度不合法！", request->request_id());
        return err_response(response->request_id(), "用户名格式不合法");
    }

    auto user = _mysql_user->SelectByNickname(nickname);
    if (user)
    {
        LOG_TRACE("{} - 用户名被占用- {}！", request->request_id(), nickname);
        return err_response(request->request_id(), "用户名被占用!");
    }

    std::string uid = Uuid();
    user = std::make_shared<User>(uid, nickname, password);
    ret = _mysql_user->Insert(user);
    if (!ret)
    {
        LOG_ERROR("{} - Mysql数据库新增数据失败!", request->request_id());
        return err_response(request->request_id(), "Mysql数据库新增数据失败!");
    }
    ret = _es_user->AppendData(uid, "", nickname, "", "");
    if (!ret)
    {
        LOG_ERROR("{} - ES搜索引擎新增数据失败!", request->request_id());
        return err_response(request->request_id(), "ES搜索引擎新增数据失败!");
    }

    response->set_request_id(request->request_id());
    response->set_success(true);
}

void UserServiceImpl::UserLogin(google::protobuf::RpcController *controller, const im::UserLoginRequest *request,
                                im::UserLoginResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard rpc_guard(done);
    LOG_TRACE("收到用户登录请求!");
    auto err_response = [&response](const std::string &rid,
                                    const std::string &errmsg)-> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };

    //1. 从请求中取出昵称和密码
    std::string nickname = request->nickname();
    std::string password = request->password();
    //2. 通过昵称获取用户信息，进行密码是否一致的判断
    auto user = _mysql_user->SelectByNickname(nickname);
    if (!user || password != user->password())
    {
        LOG_TRACE("{} - 用户名或密码错误 - {}-{}！", request->request_id(), nickname, password);
        return err_response(request->request_id(), "用户名或密码错误!");
    }
    //3. 根据 redis 中的登录标记信息是否存在判断用户是否已经登录。
    bool ret = _redis_status->Exists(user->user_id());
    if (ret == true)
    {
        LOG_TRACE("{} - 用户已在其他地方登录 - {}:{}！", request->request_id(), nickname, user->user_id());
        return err_response(request->request_id(), "用户已在其他地方登录!");
    }
    //4. 构造会话 ID，生成会话键值对，向 redis 中添加会话信息以及登录标记信息
    std::string ssid = Uuid();
    _redis_session->Append(ssid, user->user_id());
    //5. 添加用户登录信息
    _redis_status->Append(user->user_id());
    //5. 组织响应，返回生成的会话 ID
    response->set_request_id(request->request_id());
    response->set_login_session_id(ssid);
    response->set_success(true);
}

void UserServiceImpl::GetPhoneVerifyCode(google::protobuf::RpcController *controller,
                                         const im::PhoneVerifyCodeRequest *request,
                                         im::PhoneVerifyCodeResponse *response,
                                         google::protobuf::Closure *done)
{
    LOG_TRACE("收到短信验证码获取请求！");
    brpc::ClosureGuard rpc_guard(done);
    auto err_response = [response](const std::string &rid,
                                   const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };

    // 1. 从请求中取出手机号码
    std::string phone = request->phone_number();
    // 2. 验证手机号码格式是否正确（必须以 1 开始，第二位 3~9 之间，后边 9 个数字字符）
    bool ret = PhoneCheck(phone);
    if (ret == false)
    {
        LOG_ERROR("{} - 手机号码格式错误 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "手机号码格式错误!");
    }

    // 3. 生成 4 位随机验证码
    std::string code_id = Uuid();
    std::string code = VerifyCode();
    // 4. 基于短信平台 SDK 发送验证码
    ret = _dms_client->Send(phone, code);
    if (ret == false)
    {
        LOG_ERROR("{} - 短信验证码发送失败 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "短信验证码发送失败!");
    }
    // 5. 构造验证码 ID，添加到 redis 验证码映射键值索引中
    _redis_codes->Append(code_id, code);
    // 6. 组织响应，返回生成的验证码 ID
    response->set_request_id(request->request_id());
    response->set_success(true);
    response->set_verify_code_id(code_id);

    LOG_DEBUG("获取短信验证码处理完成！");
}

void UserServiceImpl::PhoneRegister(google::protobuf::RpcController *controller,
                                    const im::PhoneRegisterRequest *request, im::PhoneRegisterResponse *response,
                                    google::protobuf::Closure *done)
{
    LOG_TRACE("收到手机号注册请求!");
    brpc::ClosureGuard rpc_guard(done);

    auto err_response = [response](const std::string &rid,
                                   const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };
    // 1. 从请求中取出手机号码和验证码,验证码ID
    std::string phone = request->phone_number();
    std::string code_id = request->verify_code_id();
    std::string code = request->verify_code();
    // 2. 检查注册手机号码是否合法
    bool ret = PhoneCheck(phone);
    if (ret == false)
    {
        LOG_TRACE("{} - 手机号码格式错误 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "手机号码格式错误!");
    }
    // 3. 从 redis 数据库中进行验证码 ID-验证码一致性匹配
    auto vcode = _redis_codes->Code(code_id);
    if (vcode != code)
    {
        LOG_TRACE("{} - 验证码错误 - {}-{}！", request->request_id(), code_id, code);
        return err_response(request->request_id(), "验证码错误!");
    }
    // 4. 通过数据库查询判断手机号是否已经注册过
    auto user = _mysql_user->SelectByPhone(phone);
    if (user)
    {
        LOG_TRACE("{} - 该手机号已注册过用户 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "该手机号已注册过用户!");
    }
    // 5. 向数据库新增用户信息
    std::string uid = Uuid();
    user = std::make_shared<User>(uid, phone);
    ret = _mysql_user->Insert(user);
    if (ret == false)
    {
        LOG_TRACE("{} - 向数据库添加用户信息失败 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "向数据库添加用户信息失败!");
    }
    // 6. 向 ES 服务器中新增用户信息
    ret = _es_user->AppendData(uid, phone, uid, "", "");
    if (ret == false)
    {
        LOG_TRACE("{} - ES搜索引擎新增数据失败！", request->request_id());
        return err_response(request->request_id(), "ES搜索引擎新增数据失败！");
    }
    //7. 组织响应，进行成功与否的响应即可。
    response->set_request_id(request->request_id());
    response->set_success(true);
}

/*!
 * @bug redis和发送的验证码同时为空时，不应该登录成功
 */
void UserServiceImpl::PhoneLogin(google::protobuf::RpcController *controller, const im::PhoneLoginRequest *request,
                                 im::PhoneLoginResponse *response, google::protobuf::Closure *done)
{
    LOG_TRACE("收到手机号登录请求!");
    brpc::ClosureGuard rpc_guard(done);
    auto err_response = [response](const std::string &rid,
                                   const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };
    // 1. 从请求中取出手机号码和验证码 ID，以及验证码。
    std::string phone = request->phone_number();
    std::string code_id = request->verify_code_id();
    std::string code = request->verify_code();
    // 2. 检查注册手机号码是否合法
    bool ret = PhoneCheck(phone);
    if (ret == false)
    {
        LOG_TRACE("{} - 手机号码格式错误 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "手机号码格式错误!");
    }
    // 3. 根据手机号从数据数据进行用户信息查询，判断用用户是否存在
    auto user = _mysql_user->SelectByPhone(phone);
    if (!user)
    {
        LOG_TRACE("{} - 该手机号未注册用户 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "该手机号未注册用户!");
    }
    // 4. 从 redis 数据库中进行验证码 ID-验证码一致性匹配
    auto vcode = _redis_codes->Code(code_id);       //TODO: 如果 vcode == code == 0
    if (vcode != code)
    {
        LOG_TRACE("{} - 验证码错误 - {}-{}！", request->request_id(), code_id, code);
        return err_response(request->request_id(), "验证码错误!");
    }
    _redis_codes->Remove(code_id);
    // 5. 根据 redis 中的登录标记信息是否存在判断用户是否已经登录。
    ret = _redis_status->Exists(user->user_id());
    if (ret == true)
    {
        LOG_TRACE("{} - 用户已在其他地方登录 - {}！", request->request_id(), phone);
        return err_response(request->request_id(), "用户已在其他地方登录!");
    }
    //4. 构造会话 ID，生成会话键值对，向 redis 中添加会话信息以及登录标记信息
    std::string ssid = Uuid();
    _redis_session->Append(ssid, user->user_id());
    //5. 添加用户登录信息
    _redis_status->Append(user->user_id());
    // 7. 组织响应，返回生成的会话 ID
    response->set_request_id(request->request_id());
    response->set_login_session_id(ssid);
    response->set_success(true);
}

void UserServiceImpl::GetUserInfo(google::protobuf::RpcController *controller, const im::GetUserInfoRequest *request,
                                  im::GetUserInfoResponse *response, google::protobuf::Closure *done)
{
    LOG_TRACE("收到单个用户信息查询请求!");
    brpc::ClosureGuard rpc_guard(done);
    auto err_response = [response](const std::string &rid,
                                   const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };
    // 1. 从请求中取出用户 ID
    std::string uid = request->user_id();
    // 2. 通过用户 ID，从数据库中查询用户信息
    auto user = _mysql_user->SelectById(uid);
    if (!user)
    {
        LOG_ERROR("{} - 未找到用户信息 - {}！", request->request_id(), uid);
        return err_response(request->request_id(), "未找到用户信息!");
    }
    // 3. 根据用户信息中的头像 ID，从文件服务器获取头像文件数据，组织完整用户信息
    UserInfo *user_info = response->mutable_user_info();
    user_info->set_user_id(user->user_id());
    user_info->set_nickname(user->nickname());
    user_info->set_description(user->description());
    user_info->set_phone(user->phone());
    if (!user->avatar_id().empty())
    {
        //从信道管理对象中，获取到连接了文件管理子服务的channel
        auto channel = _channels->Choose(_file_service_name);
        if (!channel)
        {
            LOG_ERROR("{} - 未找到文件管理子服务节点 - {} - {}！", request->request_id(), _file_service_name, uid);
            return err_response(request->request_id(), "未找到文件管理子服务节点!");
        }
        //进行文件子服务的rpc请求，进行头像文件下载
        im::FileService_Stub stub(channel.get());
        im::GetSingleFileRequest req;
        im::GetSingleFileResponse rsp;
        req.set_request_id(request->request_id());
        req.set_file_id(user->avatar_id());
        brpc::Controller cntl;
        stub.GetSingleFile(&cntl, &req, &rsp, nullptr);
        if (cntl.Failed() == true || rsp.success() == false)
        {
            LOG_ERROR("{} - 文件子服务调用失败：{}！", request->request_id(), cntl.ErrorText());
            return err_response(request->request_id(), "文件子服务调用失败!");
        }
        user_info->set_avatar(rsp.file_data().file_content());
    }
    // 4. 组织响应，返回用户信息
    response->set_request_id(request->request_id());
    response->set_success(true);
}

void UserServiceImpl::GetMultiUserInfo(google::protobuf::RpcController *controller,
                                       const im::GetMultiUserInfoRequest *request,
                                       im::GetMultiUserInfoResponse *response,
                                       google::protobuf::Closure *done)
{
    LOG_TRACE("收到批量用户信息获取请求！");
    brpc::ClosureGuard rpc_guard(done);
    //1. 定义错误回调
    auto err_response = [response](const std::string &rid,
                                         const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };

    //2. 从请求中取出用户ID --- 列表
    std::vector<std::string> uid_lists;
    for (int i = 0; i < request->users_id_size(); i++)
    {
        uid_lists.push_back(request->users_id(i));
    }
    //3. 从数据库进行批量用户信息查询
    auto users = _mysql_user->SelectMultiUsers(uid_lists);
#ifdef _DEBUG
    for (auto &user: users)
    {
        LOG_TRACE("{} - 从数据库查找的用户信息 - {}！", request->request_id(), user.user_id());
    }
#endif
    if (users.size() != request->users_id_size())
    {
        LOG_ERROR("{} - 从数据库查找的用户信息数量不一致 {}-{}！",
                  request->request_id(), request->users_id_size(), users.size());
        return err_response(request->request_id(), "从数据库查找的用户信息数量不一致!");
    }

    //4. 批量从文件管理子服务进行文件下载
    auto channel = _channels->Choose(_file_service_name);
    if (!channel)
    {
        LOG_ERROR("{} - 未找到文件管理子服务节点 - {}！", request->request_id(), _file_service_name);
        return err_response(request->request_id(), "未找到文件管理子服务节点!");
    }
    im::FileService_Stub stub(channel.get());
    im::GetMultiFileRequest req;
    im::GetMultiFileResponse rsp;
    req.set_request_id(request->request_id());
    for (auto &user: users)
    {
        if (user.avatar_id().empty())
            continue;
        req.add_file_id_list(user.avatar_id());
    }

    brpc::Controller cntl;
    stub.GetMultiFile(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed() == true || rsp.success() == false)
    {
        LOG_ERROR("{} - 文件子服务调用失败：{} - {}！", request->request_id(),
                  _file_service_name, cntl.ErrorText());
        return err_response(request->request_id(), "文件子服务调用失败!");
    }
    //5. 组织响应（）
    for (auto &user: users)
    {
        auto user_map = response->mutable_users_info(); //本次请求要响应的用户信息map
        auto file_map = rsp.mutable_file_data(); //这是批量文件请求响应中的map
        UserInfo user_info;
        user_info.set_user_id(user.user_id());
        user_info.set_nickname(user.nickname());
        user_info.set_description(user.description());
        user_info.set_phone(user.phone());
        user_info.set_avatar((*file_map)[user.avatar_id()].file_content());
        (*user_map)[user_info.user_id()] = user_info;
    }
    response->set_request_id(request->request_id());
    response->set_success(true);
}

void UserServiceImpl::SetUserAvatar(google::protobuf::RpcController *controller,
                                    const im::SetUserAvatarRequest *request, im::SetUserAvatarResponse *response,
                                    google::protobuf::Closure *done)
{
    LOG_TRACE("收到用户头像设置请求！");
    brpc::ClosureGuard rpc_guard(done);
    auto err_response = [response](const std::string &rid,
                                         const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };
    // 1. 从请求中取出用户 ID 与头像数据
    std::string uid = request->user_id();
    // 2. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
    auto user = _mysql_user->SelectById(uid);
    if (!user)
    {
        LOG_TRACE("{} - {} 未找到用户信息!", request->request_id(), uid);
        return err_response(request->request_id(), "未找到用户信息!");
    }
    // 3. 上传头像文件到文件子服务，
    auto channel = _channels->Choose(_file_service_name);
    if (!channel)
    {
        LOG_ERROR("{} - 未找到文件管理子服务节点 - {}！", request->request_id(), _file_service_name);
        return err_response(request->request_id(), "未找到文件管理子服务节点!");
    }
    im::FileService_Stub stub(channel.get());
    im::PutSingleFileRequest req;
    im::PutSingleFileResponse rsp;
    req.set_request_id(request->request_id());
    req.mutable_file_data()->set_file_name("");
    req.mutable_file_data()->set_file_size(request->avatar().size());
    req.mutable_file_data()->set_file_content(request->avatar());
    brpc::Controller cntl;
    stub.PutSingleFile(&cntl, &req, &rsp, nullptr);
    if (cntl.Failed() == true || rsp.success() == false)
    {
        LOG_ERROR("{} - 文件子服务调用失败：{}！", request->request_id(), cntl.ErrorText());
        return err_response(request->request_id(), "文件子服务调用失败!");
    }
    std::string avatar_id = rsp.file_info().file_id();

    // 4. 将返回的头像文件 ID 更新到数据库中
    user->avatar_id(avatar_id);
    bool ret = _mysql_user->Update(user);
    if (ret == false)
    {
        LOG_ERROR("{} - 更新数据库用户头像ID失败 ：{}！", request->request_id(), avatar_id);
        return err_response(request->request_id(), "更新数据库用户头像ID失败!");
    }
    // 5. 更新 ES 服务器中用户信息
    ret = _es_user->AppendData(user->user_id(), user->phone(),
                               user->nickname(), user->description(), user->avatar_id());
    if (ret == false)
    {
        LOG_ERROR("{} - 更新搜索引擎用户头像ID失败 ：{}！", request->request_id(), avatar_id);
        return err_response(request->request_id(), "更新搜索引擎用户头像ID失败!");
    }
    // 6. 组织响应，返回更新成功与否
    response->set_request_id(request->request_id());
    response->set_success(true);
}

void UserServiceImpl::SetUserNickname(google::protobuf::RpcController *controller,
                                      const im::SetUserNicknameRequest *request, im::SetUserNicknameResponse *response,
                                      google::protobuf::Closure *done)
{
    LOG_TRACE("收到用户昵称设置请求！");
    brpc::ClosureGuard rpc_guard(done);
    auto err_response = [response](const std::string &rid,
                                         const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };
    // 1. 从请求中取出用户 ID 与新的昵称
    std::string uid = request->user_id();
    std::string new_nickname = request->nickname();
    // 2. 判断昵称格式是否正确
    bool ret = NicknameCheck(new_nickname);
    if (ret == false)
    {
        LOG_TRACE("{} - 用户名长度不合法！", request->request_id());
        return err_response(request->request_id(), "用户名长度不合法！");
    }
    // 3. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
    auto user = _mysql_user->SelectById(uid);
    if (!user)
    {
        LOG_TRACE("{} - 未找到用户信息 - {}！", request->request_id(), uid);
        return err_response(request->request_id(), "未找到用户信息!");
    }
    // 4. 将新的昵称更新到数据库中
    user->nickname(new_nickname);
    ret = _mysql_user->Update(user);
    if (ret == false)
    {
        LOG_ERROR("{} - 更新数据库用户昵称失败 ：{}！", request->request_id(), new_nickname);
        return err_response(request->request_id(), "更新数据库用户昵称失败!");
    }
    // 5. 更新 ES 服务器中用户信息
    ret = _es_user->AppendData(user->user_id(), user->phone(),
                               user->nickname(), user->description(), user->avatar_id());
    if (ret == false)
    {
        LOG_ERROR("{} - 更新搜索引擎用户昵称失败 ：{}！", request->request_id(), new_nickname);
        return err_response(request->request_id(), "更新搜索引擎用户昵称失败!");
    }
    // 6. 组织响应，返回更新成功与否
    response->set_request_id(request->request_id());
    response->set_success(true);
}

void UserServiceImpl::SetUserDescription(google::protobuf::RpcController *controller,
                                         const im::SetUserDescriptionRequest *request,
                                         im::SetUserDescriptionResponse *response,
                                         google::protobuf::Closure *done)
{
    LOG_TRACE("收到用户签名设置请求！");
    brpc::ClosureGuard rpc_guard(done);
    auto err_response = [response](const std::string &rid,
                                         const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };
    // 1. 从请求中取出用户 ID 与新的昵称
    std::string uid = request->user_id();
    std::string new_description = request->description();
    // 3. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
    auto user = _mysql_user->SelectById(uid);
    if (!user)
    {
        LOG_TRACE("{} - 未找到用户信息 - {}！", request->request_id(), uid);
        return err_response(request->request_id(), "未找到用户信息!");
    }
    // 4. 将新的昵称更新到数据库中
    user->description(new_description);
    bool ret = _mysql_user->Update(user);
    if (ret == false)
    {
        LOG_ERROR("{} - 更新数据库用户签名失败 ：{}！", request->request_id(), new_description);
        return err_response(request->request_id(), "更新数据库用户签名失败!");
    }
    // 5. 更新 ES 服务器中用户信息
    ret = _es_user->AppendData(user->user_id(), user->phone(),
                               user->nickname(), user->description(), user->avatar_id());
    if (ret == false)
    {
        LOG_ERROR("{} - 更新搜索引擎用户签名失败 ：{}！", request->request_id(), new_description);
        return err_response(request->request_id(), "更新搜索引擎用户签名失败!");
    }
    // 6. 组织响应，返回更新成功与否
    response->set_request_id(request->request_id());
    response->set_success(true);
}

void UserServiceImpl::SetUserPhoneNumber(google::protobuf::RpcController *controller,
                                         const im::SetUserPhoneNumberRequest *request,
                                         im::SetUserPhoneNumberResponse *response,
                                         google::protobuf::Closure *done)
{
    LOG_TRACE("收到用户手机号设置请求！");
    brpc::ClosureGuard rpc_guard(done);
    auto err_response = [response](const std::string &rid,
                                   const std::string &errmsg) -> void
    {
        response->set_request_id(rid);
        response->set_success(false);
        response->set_error(errmsg);
        return;
    };
    // 1. 从请求中取出用户 ID 与新的昵称
    std::string uid = request->user_id();
    std::string new_phone = request->phone_number();
    std::string code = request->phone_verify_code();
    std::string code_id = request->phone_verify_code_id();
    // 2. 对验证码进行验证
    auto vcode = _redis_codes->Code(code_id);
    if (vcode != code)
    {
        LOG_ERROR("{} - 验证码错误 - {}-{}！", request->request_id(), code_id, code);
        return err_response(request->request_id(), "验证码错误!");
    }
    // 3. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
    auto user = _mysql_user->SelectById(uid);
    if (!user)
    {
        LOG_ERROR("{} - 未找到用户信息 - {}！", request->request_id(), uid);
        return err_response(request->request_id(), "未找到用户信息!");
    }
    // 4. 将新的昵称更新到数据库中
    user->phone(new_phone);
    bool ret = _mysql_user->Update(user);
    if (ret == false)
    {
        LOG_ERROR("{} - 更新数据库用户手机号失败 ：{}！", request->request_id(), new_phone);
        return err_response(request->request_id(), "更新数据库用户手机号失败!");
    }
    // 5. 更新 ES 服务器中用户信息
    ret = _es_user->AppendData(user->user_id(), user->phone(),
                               user->nickname(), user->description(), user->avatar_id());
    if (ret == false)
    {
        LOG_ERROR("{} - 更新搜索引擎用户手机号失败 ：{}！", request->request_id(), new_phone);
        return err_response(request->request_id(), "更新搜索引擎用户手机号失败!");
    }
    // 6. 组织响应，返回更新成功与否
    response->set_request_id(request->request_id());
    response->set_success(true);
}

UserServer::UserServer(const Discovery::Ptr &service_discoverer, const Register::Ptr &reg_client,
                       const std::shared_ptr<elasticlient::Client> &es_client,
                       const std::shared_ptr<odb::core::database> &mysql_client,
                       const std::shared_ptr<sw::redis::Redis> &redis_client,
                       const std::shared_ptr<brpc::Server> &server) :
    _service_discoverer(service_discoverer),
    _registry_client(reg_client),
    _es_client(es_client),
    _mysql_client(mysql_client),
    _redis_client(redis_client),
    _rpc_server(server)
{
}

UserServer::~UserServer() = default;

void UserServer::Start() const
{
    _rpc_server->RunUntilAskedToQuit();
}

void UserServerBuilder::MakeESObject(const std::vector<std::string> &host_list)
{
    _es_client = ESClientFactory::Create(host_list);
}

void UserServerBuilder::MakeDmsObject(std::string &access_key)
{
    _dms_client = std::make_shared<DMSClient>(access_key);
}

void UserServerBuilder::MakeMysqlObject(const std::string &user, const std::string &pswd, const std::string &host,
        const std::string &db, const std::string &cset, int port, int conn_pool_count)
{
    _mysql_client = ODBFactory::Create(user, pswd, host, db, cset, port, conn_pool_count);
}

void UserServerBuilder::MakeRedisObject(const std::string &host, int port, int db, bool keep_alive)
{
    _redis_client = RedisClientFactory::Create(host, port, db, keep_alive);
    try
    {
        LOG_INFO("REDIS PING: {}", _redis_client->ping());
    } catch (std::exception ex)
    {
        LOG_ERROR("redis server connect error: {}", ex.what());
        abort();
    }
}

void UserServerBuilder::MakeDiscoveryObject(const std::string &reg_host, const std::string &base_service_name,
        const std::string &file_service_name)
{
    _file_service_name = file_service_name;
    _channels = std::make_shared<ServiceManager>();
    _channels->Declared(file_service_name);
    LOG_DEBUG("设置文件子服务为需添加管理的子服务：{}", file_service_name);
    auto put_cb = std::bind(&ServiceManager::OnServiceOnline, _channels.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&ServiceManager::OnServiceOffline, _channels.get(), std::placeholders::_1, std::placeholders::_2);
    _service_discoverer = std::make_shared<Discovery>(reg_host, base_service_name, put_cb, del_cb);
}

void UserServerBuilder::MakeRegistryObject(const std::string &reg_host, const std::string &service_name,
        const std::string &access_host)
{
    _registry_client = std::make_shared<Register>(reg_host);
    _registry_client->Registry(service_name, access_host);
}

void UserServerBuilder::MakeRpcServer(uint16_t port, int32_t timeout, uint8_t num_threads)
{
    if (!_es_client) {
        LOG_ERROR("还未初始化ES搜索引擎模块！");
        abort();
    }
    if (!_mysql_client) {
        LOG_ERROR("还未初始化Mysql数据库模块！");
        abort();
    }
    if (!_redis_client) {
        LOG_ERROR("还未初始化Redis数据库模块！");
        abort();
    }
    if (!_channels) {
        LOG_ERROR("还未初始化信道管理模块！");
        abort();
    }
    if (!_dms_client) {
        LOG_ERROR("还未初始化短信平台模块！");
        abort();
    }
    _rpc_server = std::make_shared<brpc::Server>();

    UserServiceImpl *user_service = new UserServiceImpl(_dms_client, _es_client,
    _mysql_client, _redis_client, _channels, _file_service_name);
    int ret = _rpc_server->AddService(user_service,
        brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
    if (ret == -1) {
        LOG_ERROR("添加Rpc服务失败！");
        abort();
    }
    brpc::ServerOptions options;
    options.idle_timeout_sec = timeout;
    options.num_threads = num_threads;
    ret = _rpc_server->Start(port, &options);
    if (ret == -1) {
        LOG_ERROR("服务启动失败！");
        abort();
    }
}

UserServer::Ptr UserServerBuilder::Build()
{
    if (!_service_discoverer) {
        LOG_ERROR("还未初始化服务发现模块！");
        abort();
    }
    if (!_registry_client) {
        LOG_ERROR("还未初始化服务注册模块！");
        abort();
    }
    if (!_rpc_server) {
        LOG_ERROR("还未初始化RPC服务器模块！");
        abort();
    }
    UserServer::Ptr server = std::make_shared<UserServer>(
        _service_discoverer, _registry_client,
        _es_client, _mysql_client, _redis_client, _rpc_server);
    return server;
}
