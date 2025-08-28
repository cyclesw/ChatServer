#include "mysql_apply.h"
#include "friend_apply-odb.hxx"
#include "friend_apply.hxx"
#include "log.hpp"

#include <memory>
#include <odb/database.hxx>

namespace im
{
    FriendApplyTable::FriendApplyTable(const std::shared_ptr<odb::core::database> &db) : _db(db)
    {
    }

    bool FriendApplyTable::Insert(FriendApply &apply)
    {
        try
        {
            odb::transaction trans(_db->begin());
            _db->persist(apply);
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("新增好友申请事件失败 {}-{}:{}！", apply.user_id(), apply.peer_id(), e.what());
            return false;
        }
        return true;
    }

    bool FriendApplyTable::Exists(const std::string &uid, const std::string &pid)
    {
        bool flag = false;
        try
        {
            typedef odb::query<FriendApply> query;
            typedef odb::result<FriendApply> result;
            odb::transaction trans(_db->begin());
            result r(_db->query<FriendApply>(query::user_id == uid && query::peer_id == pid));
            LOG_DEBUG("{} - {} 好友事件数量：{}", uid, pid, r.size());
            flag = !r.empty();
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("获取好友申请事件失败:{}-{}-{}！", uid, pid, e.what());
        }
        return flag;
    }

    bool FriendApplyTable::Remove(const std::string &uid, const std::string &pid)
    {
        try
        {
            odb::transaction trans(_db->begin());
            typedef odb::query<FriendApply> query;
            typedef odb::result<FriendApply> result;
            _db->erase_query<FriendApply>(query::user_id == uid && query::peer_id == pid);
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("删除好友申请事件失败 {}-{}:{}！", uid, pid, e.what());
            return false;
        }
        return true;
    }

    std::vector<std::string> FriendApplyTable::ApplyUsers(const std::string &uid)
    {
        std::vector<std::string> res;
        try
        {
            odb::transaction trans(_db->begin());
            typedef odb::query<FriendApply> query;
            typedef odb::result<FriendApply> result;
            // 当前的uid是被申请者的用户ID
            result r(_db->query<FriendApply>(query::peer_id == uid));
            for (result::iterator i(r.begin()); i != r.end(); ++i)
            {
                res.push_back(i->user_id());
            }
            trans.commit();
        }
        catch (std::exception &e)
        {
            LOG_ERROR("通过用户{}的好友申请者失败:{}！", uid, e.what());
        }
        return res;
    }

} // namespace im
