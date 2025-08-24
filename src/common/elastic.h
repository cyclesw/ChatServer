#pragma once

#include "icsearch.h"

#include <elasticlient/client.h>
#include <memory>

namespace im
{
class Message;
class User;

class ESClientFactory
    {
    public:
        static std::shared_ptr<elasticlient::Client> Create(std::vector<std::string> host_list)
        {
            return std::make_shared<elasticlient::Client>(host_list);
        }
    };

class ESUser
{
public:
    using Ptr = std::shared_ptr<ESUser>;

    explicit ESUser(const std::shared_ptr<elasticlient::Client>& client);

    bool CreateIndex();

    bool AppendData(const std::string& uid,
        const std::string& phone,
        const std::string& nickname,
        const std::string& description,
        const std::string& avatar_id);

    std::vector<User> Search(const std::string& key, const std::vector<std::string>& uid_list);
private:
    std::shared_ptr<elasticlient::Client> _es_client;
};

class ESMessage
{
public:
    using Ptr = std::shared_ptr<ESMessage>;
    ESMessage(const std::shared_ptr<elasticlient::Client>& es_client);

    bool CreateIndex();

    bool AppendData(const std::string& user_id,
        const std::string& message_id,
        const long create_time,
        const std::string& chat_session_id,
        const std::string& content);

    bool Remove(const std::string& mid);

    std::vector<im::Message> Search(const std::string& key, const std::string& ssid);
private:
    std::shared_ptr<elasticlient::Client> _es_client;
};
}