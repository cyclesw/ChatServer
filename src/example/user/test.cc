#include <gflags/gflags.h>
#include "../../common/database/user_table.hpp"
#include "user-odb.hxx"
#include "database/mysql.hpp"
#include "user.hxx"
#include "database/user_table.hpp"
#include "log.hpp"

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");


void insert(im::UserTable &user)
{
    auto user1 = std::make_shared<im::User>("uid1", "昵称1", "123456");
    user.Insert(user1);

    auto user2 = std::make_shared<im::User>("uid2", "15566667777");
    user.Insert(user2);
}

void update_by_id(im::UserTable& user_tb)
{
    auto user = user_tb.SelectById("uid1");
    user->description("都选C");
    user_tb.Update(user);
}

void update_by_phone(im::UserTable& user_tb)
{
    auto user = user_tb.SelectByPhone("15566667777");
    user->password("22223333");
    user_tb.Update(user);
}

void update_by_nickname(im::UserTable& user_tb)
{
    auto user = user_tb.SelectByNickname("昵称2");
    user->nickname("昵称2");
    user_tb.Update(user);

}

void select_users(im::UserTable& user_tb)
{
    std::vector<std::string> id_list = { "uid1", "uid2" };
    auto users = user_tb.SelectMultiUsers(id_list);
    for (auto user: users)
        LOG_INFO("nickname: {}", user.nickname());
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    auto db = im::ODBFactory::Create("test", "123456", "127.0.0.1", "TestDB", "utf8", 0, 1);

    im::UserTable user(db);

    // insert(user);
    update_by_id(user);
    update_by_phone(user);
    update_by_nickname(user);
    select_users(user);
    return 0;
}
