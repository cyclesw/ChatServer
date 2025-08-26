#include "database/mysql.hpp"
// #include "chat_session_member.hxx"
#include "database/mysql_chat_session_member.h"
#include "log.hpp"
#include <gflags/gflags.h>
#include <iostream>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");


void append_test(im::ChatSessionMemberTable &tb) {
    im::ChatSessionMember csm1("会话ID1", "用户ID1");
    tb.Append(csm1);
    im::ChatSessionMember csm2("会话ID1", "用户ID2");
    tb.Append(csm2);
    im::ChatSessionMember csm3("会话ID2", "用户ID3");
    tb.Append(csm3);
    im::ChatSessionMember csm4("会话ID2", "用户ID2");
    tb.Append(csm4);
}

void multi_append_test(im::ChatSessionMemberTable &tb) {
    im::ChatSessionMember csm1("会话ID3", "用户ID1");
    im::ChatSessionMember csm2("会话ID3", "用户ID2");
    im::ChatSessionMember csm3("会话ID3", "用户ID3");
    std::vector<im::ChatSessionMember> list = {csm1, csm2, csm3};
    tb.Append(list);
}

void remove_test(im::ChatSessionMemberTable &tb) {
    im::ChatSessionMember csm3("会话ID2", "用户ID3");
    tb.Remove(csm3);
}

void ss_members(im::ChatSessionMemberTable &tb) {
    auto res = tb.Members("会话ID3");
    for (auto &id : res) {
        LOG_INFO("会话ID3 成员: {}", id);
    }
}
void remove_all(im::ChatSessionMemberTable &tb) {
    // tb.Remove("会话ID1");
    tb.Remove("会话ID2");
    tb.Remove("会话ID3");
}


int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    auto db = im::ODBFactory::Create("test", "123456", "127.0.0.1", "im", "utf8", 0, 1);
    
    im::ChatSessionMemberTable csmt(db);
    append_test(csmt);
    multi_append_test(csmt);
    remove_test(csmt);
    ss_members(csmt);
    remove_all(csmt);
    return 0;
}