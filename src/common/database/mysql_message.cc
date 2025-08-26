#include "mysql_message.h"

#include "message-odb.hxx"
#include <odb/database.hxx>

#include "log.hpp"


namespace im
{
    MessageTable::MessageTable(const std::shared_ptr<odb::core::database>& db)
        :_db(db)
    {
        
    }

    bool MessageTable::Insert(Message& msg)
    {
        try {
            odb::transaction trans(_db->begin());
            _db->persist(msg);
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("新增消息失败 {}:{}！", msg.message_id(),e.what());
            return false;
        }
        return true;
    }
    
    bool MessageTable::Remove(const std::string& ssid)
    {
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<Message> query;
            typedef odb::result<Message> result;
            _db->erase_query<Message>(query::session_id == ssid);
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("删除会话所有消息失败 {}:{}！", ssid, e.what());
            return false;
        }
        return true;
    }
    
    std::vector<Message> MessageTable::Recent(const std::string& ssid, int count)
    {
        std::vector<Message> res;
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<Message> query;
            typedef odb::result<Message> result;
            //本次查询是以ssid作为过滤条件，然后进行以时间字段进行逆序，通过limit
            // session_id='xx' order by create_time desc  limit count;
            std::stringstream cond;
            cond << "session_id='" << ssid << "' ";
            cond << "order by create_time desc limit " << count;
            result r(_db->query<Message>(cond.str()));
            for (result::iterator i(r.begin()); i != r.end(); ++i) {
                res.push_back(*i);
            }
            std::reverse(res.begin(), res.end());
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("获取最近消息失败:{}-{}-{}！", ssid, count, e.what());
        }
        return res;
    }
    
    std::vector<Message> MessageTable::Range(const std::string& ssid,
                boost::posix_time::ptime& stime,
                boost::posix_time::ptime& etime)
    {
        std::vector<Message> res;
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<Message> query;
            typedef odb::result<Message> result;
            //获取指定会话指定时间段的信息
            result r(_db->query<Message>(query::session_id == ssid && 
                query::create_time >= stime &&
                query::create_time <= etime));
            for (result::iterator i(r.begin()); i != r.end(); ++i) {
                res.push_back(*i);
            }
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("获取区间消息失败:{}-{}:{}-{}！", ssid, 
                boost::posix_time::to_simple_string(stime), 
                boost::posix_time::to_simple_string(etime), e.what());
        }
        return res;
    }
}