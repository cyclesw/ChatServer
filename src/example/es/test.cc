#include "icsearch.h"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, true, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(es_host, "http://127.0.0.1:9200/", "es服务器URL");

int main(int argc, char** argv)
{
    using namespace im::logger;
    google::ParseCommandLineFlags(&argc, &argv, true);

    LogSetting setting;
    setting.level = static_cast<Level>(FLAGS_log_level);
    setting.logger_type = FLAGS_run_mode ? LogType::Console : LogType::File;
    setting.logger_name = "gateway";
    InitLogger(setting);


    std::vector<std::string> host_list = {"http://127.0.0.1:9200/"};
    auto client = std::make_shared<elasticlient::Client>(host_list);
    LOG_INFO("{}", (void*)client.get());

    bool ret = im::ESIndex(client, "test_user").Append("nickname")
        .Append("phone", "keyword", "standard", true)
        .Create();

    if (!ret)
    {
        LOG_INFO("索引创建失败!");
        return -1;
    }
    else
    {
        LOG_INFO("索引创建成功!");
    }

    ret = im::ESInsert(client, "test_user")
        .Append("nickname", "张三")
        .Append("phone", "13800000000")
        .Insert("00002");
    if (ret == false) {
        LOG_ERROR("数据插入失败!");
        return -1;
    }else {
        LOG_INFO("数据新增成功!");
    }
    //数据的修改
    ret = im::ESInsert(client, "test_user")
        .Append("nickname", "张三")
        .Append("phone", "13344445555")
        .Insert("00001");
    if (ret == false) {
        LOG_ERROR("数据更新失败!");
        return -1;
    }else {
        LOG_INFO("数据更新成功!");
    }

    // 新增三个数据插入操作
    ret = im::ESInsert(client, "test_user")
        .Append("nickname", "李四")
        .Append("phone", "13911111111")
        .Insert("00003");
    if (ret == false) {
        LOG_ERROR("数据插入失败!");
        return -1;
    }else {
        LOG_INFO("数据新增成功!");
    }

    ret = im::ESInsert(client, "test_user")
        .Append("nickname", "王五")
        .Append("phone", "13722222222")
        .Insert("00004");
    if (ret == false) {
        LOG_ERROR("数据插入失败!");
        return -1;
    }else {
        LOG_INFO("数据新增成功!");
    }

    ret = im::ESInsert(client, "test_user")
        .Append("nickname", "赵六")
        .Append("phone", "13633333333")
        .Insert("00005");
    if (ret == false) {
        LOG_ERROR("数据插入失败!");
        return -1;
    }else {
        LOG_INFO("数据新增成功!");
    }

    Json::Value user = im::ESSearch(client, "test_user")
    // .AppendShouldMatch("nickname.keyword", "张三")
    .AppendMustNotTerms("phone.keyword", {"13344445555"})
    .Search();
    if (user.empty() || user.isArray() == false) {
        LOG_ERROR("结果为空，或者结果不是数组类型");
        return -1;
    } else {
        LOG_INFO("数据检索成功!");
    }
    int sz = user.size();
    LOG_DEBUG("检索结果条目数量：{}", sz);
    for (int i = 0; i < sz; i++) {
        LOG_INFO("nickname: {}", user[i]["_source"]["nickname"].asString());
    }

    // ret = im::ESRemove(client, "test_user").Remove("00001");
    // if (ret == false) {
    //     LOG_ERROR("删除数据失败");
    //     return -1;
    // }  else {
    //     LOG_INFO("数据删除成功!");
    // }

    return 0;
}