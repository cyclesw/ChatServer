#pragma once
#include "log.hpp"
#include "user.hxx"
#include "user-odb.hxx"
#include "mysql.hpp"

#include <memory>


namespace im
{
class UserTable
{
public:
    using Ptr = std::shared_ptr<UserTable>;
    UserTable(const std::shared_ptr<odb::core::database>& db)
        :_db(db)
    {
    }

    bool Insert(const std::shared_ptr<User> &user)
    {
        try {
            odb::transaction trans(_db->begin());
            _db->persist(*user);
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("新增用户失败 {}:{}！", user->nickname(),e.what());
            return false;
        }
        return true;
    }

    bool Update(const std::shared_ptr<User>& user)
    {
        try
        {
            odb::transaction trans(_db->begin());
            _db->update(*user);
            trans.commit();
        }
        catch (std::exception& e)
        {
            LOG_ERROR("更新用户失败: {}-{}!", user->nickname(), e.what());
            return false;
        }
        return true;
    }

    std::shared_ptr<User> SelectByNickname(const std::string& nickname)
    {
        std::shared_ptr<User> res;
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<User> query;
            typedef odb::result<User> result;
            res.reset(_db->query_one<User>(query::nickname == nickname));
            trans.commit();
        }catch (std::exception &e) {
            LOG_ERROR("通过昵称查询用户失败 {}:{}！", nickname, e.what());
        }
        return res;

    }

    // std::shared_ptr<User> SelectByPhone(const std::string& phone)
    // {}
    //
    // std::shared_ptr<User> SelectById(const std::string& user_id)
    // {}
    //
    // std::vector<User> SelectMultiUsers(const std::vector<std::string>& id_list)
    // {}
private:
    std::shared_ptr<odb::core::database> _db;
};
}