#include "user_table.hpp"

#include "user-odb.hxx"

#include "mysql.hpp"
#include "user.hxx"
#include "log.hpp"

#include <odb/database.hxx>
#include <sstream>


im::UserTable::UserTable(const std::shared_ptr<odb::core::database> &db)
:_db(db)
{
}
bool im::UserTable::Insert(const std::shared_ptr<User> &user)
{
    try
    {
        odb::transaction trans(_db->begin());
        _db->persist(*user);
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("新增用户失败 {}:{}!", user->nickname(), e.what());
        return false;
    }
    return true;
}
bool im::UserTable::Update(const std::shared_ptr<User> &user)
{
    try
    {
        odb::transaction trans(_db->begin());
        _db->update(*user);
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("更新用户失败: {}-{}!", user->nickname(), e.what());
        return false;
    }
    return true;
}
std::shared_ptr<im::User> im::UserTable::SelectByNickname(const std::string &nickname) const
{
    std::shared_ptr<User> res;
    try
    {
        odb::transaction trans(_db->begin());
        typedef odb::query<User> query;
        typedef odb::result<User> result;
        res.reset(_db->query_one<User>(query::nickname == nickname));
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("通过昵称查询用户失败 {}:{}!", nickname, e.what());
    }
    return res;
}
std::shared_ptr<im::User> im::UserTable::SelectByPhone(const std::string &phone) const
{
    std::shared_ptr<User> res;
    try
    {
        odb::transaction trans(_db->begin());
        typedef odb::query<User> query;
        typedef odb::result<User> result;
        res.reset(_db->query_one<User>(query::phone == phone));
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("通过手机号查询用户失败 {}:{}!", phone, e.what());
    }
    return res;
}
std::shared_ptr<im::User> im::UserTable::SelectById(const std::string &user_id) const
{
    std::shared_ptr<User> res;
    try
    {
        odb::transaction trans(_db->begin());
        typedef odb::query<User> query;
        typedef odb::result<User> result;
        res.reset(_db->query_one<User>(query::user_id == user_id));
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("通过用户ID查询用户失败 {}:{}!", user_id, e.what());
    }
    return res;
}
std::vector<im::User> im::UserTable::SelectMultiUsers(const std::vector<std::string> &id_list) const
{
    // select * from user where id in ('id1', 'id2', ...)
    if (id_list.empty())
    {
        return std::vector<User>();
    }
    std::vector<User> res;
    try
    {
        odb::transaction trans(_db->begin());
        typedef odb::query<User> query;
        typedef odb::result<User> result;
        std::stringstream ss;
        ss << "user_id in (";
        for (const auto &id: id_list)
        {
            ss << "'" << id << "',";
        }
        std::string condition = ss.str();
        condition.pop_back();
        condition += ")";
        result r(_db->query<User>(condition));
        for (result::iterator i(r.begin()); i != r.end(); ++i)
        {
            res.push_back(*i);
        }
        trans.commit();
    }
    catch (std::exception &e)
    {
        LOG_ERROR("通过用户ID批量查询用户失败:{}!", e.what());
    }
    return res;
}

