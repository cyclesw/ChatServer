#include "mysql_chat_session_member.h"

#include "chat_session_member.hxx"
#include "chat_session_member-odb.hxx"

#include "log.hpp"

#include <odb/database.hxx>



im::ChatSessionMemberTable::ChatSessionMemberTable(const std::shared_ptr<odb::core::database>& db)
    :_db(db)
{
}
bool im::ChatSessionMemberTable::Append(ChatSessionMember &csm)
{
    try {
        odb::transaction trans(_db->begin());
        _db->persist(csm);
        trans.commit();
    }catch (std::exception &e) {
        LOG_ERROR("新增单会话成员失败 {}-{}:{}！",
            csm.session_id(), csm.user_id(), e.what());
        return false;
    }
    return true;
}
bool im::ChatSessionMemberTable::Append(std::vector<ChatSessionMember> &csm_lists)
{
    try {
        odb::transaction trans(_db->begin());
        for (auto &csm : csm_lists) {
            _db->persist(csm);
        }
        trans.commit();
    }catch (std::exception &e) {
        LOG_ERROR("新增多会话成员失败 {}-{}:{}！",
            csm_lists[0].session_id(), csm_lists.size(), e.what());
        return false;
    }
    return true;
}
bool im::ChatSessionMemberTable::Remove(ChatSessionMember &csm)
{
    try
    {
        odb::transaction trans(_db->begin());
        typedef odb::query<ChatSessionMember> query;
        typedef odb::result<ChatSessionMember> result;
        _db->erase_query<ChatSessionMember>(query::session_id == csm.session_id() && query::user_id == csm.user_id());
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("删除单会话成员失败 {}-{}:{}！", csm.session_id(), csm.user_id(), e.what());
        return false;
    }
    return true;
}
bool im::ChatSessionMemberTable::Remove(const std::string &ssid)
{
    try
    {
        odb::transaction trans(_db->begin());
        typedef odb::query<ChatSessionMember> query;
        typedef odb::result<ChatSessionMember> result;
        _db->erase_query<ChatSessionMember>(query::session_id == ssid);
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("删除会话所有成员失败 {}:{}！", ssid, e.what());
        return false;
    }
    return true;
}
std::vector<std::string> im::ChatSessionMemberTable::members(const std::string &ssid)
{
    std::vector<std::string> res;
    try
    {
        odb::transaction trans(_db->begin());
        typedef odb::query<ChatSessionMember> query;
        typedef odb::result<ChatSessionMember> result;
        result r(_db->query<ChatSessionMember>(query::session_id == ssid));
        for (result::iterator i(r.begin()); i != r.end(); ++i)
        {
            res.push_back(i->user_id());
        }
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("获取会话成员失败:{}-{}！", ssid, e.what());
    }
    return res;
}
