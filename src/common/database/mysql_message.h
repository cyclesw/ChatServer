#pragma once
#include <memory>
#include <odb/forward.hxx>
#include <string>
#include <vector>
#include "message.hxx"

namespace im 
{
    class MessageTable 
    {
    public:
        using Ptr = std::shared_ptr<MessageTable>;

        MessageTable(const std::shared_ptr<odb::core::database>& db);

        bool Insert(Message& msg);

        bool Remove(const std::string& ssid);

        std::vector<Message> Recent(const std::string& ssid, int count);

        std::vector<Message> Range(const std::string& ssid,
            boost::posix_time::ptime& stime,
            boost::posix_time::ptime& etime);
            
    private:
        std::shared_ptr<odb::core::database> _db;
    };
}