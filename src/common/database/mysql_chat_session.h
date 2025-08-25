#pragma once

#include <memory>
#include <vector>
#include <odb/forward.hxx>
#include "chat_session.hxx"

namespace im
{
    class ChatSessionTable
    {
    public:
        using Ptr = std::shared_ptr<ChatSessionTable>;

        ChatSessionTable(const std::shared_ptr<odb::core::database>& db);

        bool Insert(ChatSession &cs);

        bool Remove(const std::string& ssid);

        bool Remove(const std::string& uid, const std::string& pid);

        std::shared_ptr<ChatSession> Select(const std::string& ssid);

        std::vector<SingleChatSession> SingleChatSession(const std::string& uid);

        std::vector<GroupChatSession> GroupChatSession(const std::string &uid);
    private:
        std::shared_ptr<odb::core::database> _db;
    };
}