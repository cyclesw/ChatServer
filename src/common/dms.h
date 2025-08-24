#pragma once
#include <memory>

//TODO: 不同平台适配

namespace im
{
class DMSClient
{
public:
    using Ptr = std::shared_ptr<DMSClient>;
    // Spug平台
    DMSClient(const std::string& access_key);

    ~DMSClient();

    bool Send(const std::string& phone, const std::string& code);
};
}