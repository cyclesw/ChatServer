#pragma once
#include "chat_session_member.hxx"
#include "log.hpp"
#include "mysql.hpp"

namespace im
{
    class ChatSessionMemberTable
    {
    public:
        using Ptr = std::shared_ptr<ChatSessionMemberTable>;

        ChatSessionMemberTable(const std::shared_ptr<odb::core::database>& db);

        bool Append(ChatSessionMember& csm);

        bool Append(std::vector<ChatSessionMember>& csm_lists);

        //删除指定会话中的指定成员 -- ssid & uid
        bool Remove(ChatSessionMember &csm);
        // 删除会话的所有成员信息
        bool Remove(const std::string &ssid);

        std::vector<std::string> members(const std::string &ssid);

    private:
        std::shared_ptr<odb::core::database> _db;
    };
}