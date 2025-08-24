#include "dms.h"
#include "log.hpp"

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#endif


#include <httplib.h>
#include <string>


using namespace im;

static std::string key;

DMSClient::DMSClient(const std::string &access_key)
{
    key = access_key;
}

DMSClient::~DMSClient() = default;

bool DMSClient::Send(const std::string& phone, const std::string& code)
{
    std::stringstream ss;
    ss << "/send/";
    ss << key;

    httplib::Client client("https://push.spug.cc");

    httplib::Params params {
    {"code",    code.c_str()},
    {"targets", phone.c_str()}
    };

    auto res = client.Post(ss.str(),
                            httplib::Headers{},
                            params);

    if (res && res->status == 200)
    {
        LOG_TRACE("短信请求成功: {}", res->body);
        return true;
    }
    else if (res)
    {
        LOG_ERROR("短信请求失败: {}-{}", res->status, res->body);
        return false;
    }

    LOG_ERROR("短信请求失败: API内部错误");
    return false;
}