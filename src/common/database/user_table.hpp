#pragma once

#include <memory>
#include <vector>

namespace odb
{
    class database;

    namespace core
    {
        using odb::database;              // 别名，不增加新类型
    }
}


namespace im
{
class User;

class UserTable
{
public:
    using Ptr = std::shared_ptr<UserTable>;
    UserTable(const std::shared_ptr<odb::core::database>& db);

    bool Insert(const std::shared_ptr<User> &user);

    bool Update(const std::shared_ptr<User>& user);

    std::shared_ptr<User> SelectByNickname(const std::string& nickname) const;

    std::shared_ptr<User> SelectByPhone(const std::string& phone) const;

    std::shared_ptr<User> SelectById(const std::string& user_id) const;

    std::vector<User> SelectMultiUsers(const std::vector<std::string>& id_list) const;

private:
    std::shared_ptr<odb::core::database> _db;
};
}