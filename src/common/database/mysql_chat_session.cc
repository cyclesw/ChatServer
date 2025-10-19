#include "mysql_chat_session.h"
#include <odb/database.hxx>
#include <odb/core.hxx>
#include <odb/query.hxx>
#include "chat_session-odb.hxx"
#include "chat_session.hxx"
#include "mysql_chat_session_member.h"

#include "mysql.hpp"
#include "log.hpp"

namespace im
{
    ChatSessionTable::ChatSessionTable(const std::shared_ptr<odb::core::database>& db)
        :_db(db)
    {
        
    }
    
    bool ChatSessionTable::Insert(ChatSession &cs)
    {
        try {
            odb::transaction trans(_db->begin());
            _db->persist(cs);
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("新增会话失败 {}:{}！", cs.chat_session_name(), e.what());
            return false;
        }
        return true;
    }

    bool ChatSessionTable::Insert(ChatSession &&cs)
    {
        try {
            odb::transaction trans(_db->begin());
            _db->persist(cs);
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("新增会话失败 {}:{}！", cs.chat_session_name(), e.what());
            return false;
        }
        return true;
    }

    bool ChatSessionTable::Remove(const std::string& csid)
    {
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<ChatSession> query;
            typedef odb::result<ChatSession> result;
            _db->erase_query<ChatSession>(query::chat_session_id == csid);

            typedef odb::query<ChatSessionMember> mquery;
            _db->erase_query<ChatSessionMember>(mquery::session_id == csid);
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("删除会话失败 {}:{}！", csid, e.what());
            return false;
        }
        return true;

    }
    
    bool ChatSessionTable::Remove(const std::string& uid, const std::string& pid)
    {
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<im::SingleChatSession> query;
            typedef odb::result<im::SingleChatSession> result;
            auto res = _db->query_one<im::SingleChatSession>(
                query::csm1::user_id == uid && 
                query::csm2::user_id == pid && 
                query::css::chat_session_type == ChatSessionType::SINGLE);

            std::string cssid = res->chat_session_id;
            typedef odb::query<ChatSession> cquery;
            _db->erase_query<ChatSession>(cquery::chat_session_id == cssid);

            typedef odb::query<ChatSessionMember> mquery;
            _db->erase_query<ChatSessionMember>(mquery::session_id == cssid);
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("删除会话失败 {}-{}:{}！", uid, pid, e.what());
            return false;
        }
        return true;
    }

    bool ChatSessionTable::Update(const std::shared_ptr<ChatSession> &cs)
    {
        try
        {
            odb::transaction trans(_db->begin());
            _db->update(*cs);
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("更新会话失败: {}-{}!", cs->chat_session_name(), e.what());
            return false;
        }
        return true;
    }

    std::shared_ptr<ChatSession> ChatSessionTable::Select(const std::string& csid)
    {
        std::shared_ptr<ChatSession> res;
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<ChatSession> query;
            typedef odb::result<ChatSession> result;
            res.reset(_db->query_one<ChatSession>(query::chat_session_id == csid));
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("通过会话ID获取会话信息失败 {}:{}！", csid, e.what());
        }
        return res;
    }
    
    std::vector<SingleChatSession> ChatSessionTable::GetSingleChatSession(const std::string& uid)
    {
         std::vector<im::SingleChatSession> res;
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<im::SingleChatSession> query;
            typedef odb::result<im::SingleChatSession> result;
            //当前的uid是被申请者的用户ID
            result r(_db->query<im::SingleChatSession>(
                query::css::chat_session_type == ChatSessionType::SINGLE && 
                query::csm1::user_id == uid && 
                query::csm2::user_id != query::csm1::user_id));
            for (result::iterator i(r.begin()); i != r.end(); ++i) {
                res.push_back(*i);
            }
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("获取用户 {} 的单聊会话失败:{}！", uid, e.what());
        }
        return res;
       
    }
    
    std::vector<GroupChatSession> ChatSessionTable::GetGroupChatSession(const std::string &uid)
    {
        std::vector<im::GroupChatSession> res;
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<im::GroupChatSession> query;
            typedef odb::result<im::GroupChatSession> result;
            //当前的uid是被申请者的用户ID
            result r(_db->query<im::GroupChatSession>(
                query::css::chat_session_type == ChatSessionType::GROUP && 
                query::csm::user_id == uid ));
            for (result::iterator i(r.begin()); i != r.end(); ++i) {
                res.push_back(*i);
            }
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("获取用户 {} 的群聊会话失败:{}！", uid, e.what());
        }
        return res;
    }
}