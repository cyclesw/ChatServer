#include "mysql_relation.h"
#include "relation-odb.hxx"

#include "log.hpp"

#include <odb/database.hxx>

namespace im
{
    RelationTable::RelationTable(const std::shared_ptr<odb::core::database> &db) : _db(db)
    {
    }

    bool RelationTable::Insert(const std::string &uid, const std::string &pid)
    {
        //{1,2} {2,1}
        try
        {
            Relation r1(uid, pid);
            Relation r2(pid, uid);
            odb::transaction trans(_db->begin());
            _db->persist(r1);
            _db->persist(r2);
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("新增用户好友关系信息失败 {}-{}:{}！", uid, pid, e.what());
            return false;
        }
        return true;
    }

    bool RelationTable::Remove(const std::string &uid, const std::string &pid)
    {
        try
        {
            odb::transaction trans(_db->begin());
            typedef odb::query<Relation> query;
            typedef odb::result<Relation> result;
            _db->erase_query<Relation>(query::user_id == uid && query::peer_id == pid);
            _db->erase_query<Relation>(query::user_id == pid && query::peer_id == uid);
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("删除好友关系信息失败 {}-{}:{}！", uid, pid, e.what());
            return false;
        }
        return true;
    }

    bool RelationTable::Exists(const std::string &uid, const std::string &pid)
    {
        typedef odb::query<Relation> query;
        typedef odb::result<Relation> result;
        result r;
        bool flag = false;
        try
        {
            odb::transaction trans(_db->begin());
            r = _db->query<Relation>(query::user_id == uid && query::peer_id == pid);
            flag = !r.empty();
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("获取用户好友关系失败:{}-{}-{}！", uid, pid, e.what());
        }
        return flag;
    }

    std::vector<std::string> RelationTable::Friends(const std::string &uid)
    {
        std::vector<std::string> res;
        try
        {
            odb::transaction trans(_db->begin());
            typedef odb::query<Relation> query;
            typedef odb::result<Relation> result;
            result r(_db->query<Relation>(query::user_id == uid));
            for (result::iterator i(r.begin()); i != r.end(); ++i)
            {
                res.push_back(i->peer_id());
            }
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("通过用户-{}的所有好友ID失败:{}！", uid, e.what());
        }
        return res;
    }
} // namespace im
