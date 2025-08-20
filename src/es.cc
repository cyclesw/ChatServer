#include "elastic.h"
#include "icsearch.h"
#include "message.hxx"
#include "user_table.hxx"
#include <boost/date_time/posix_time/conversion.hpp>
#include <vector>

namespace im
{
bool Serialize(const Json::Value& val, std::string& dst)
{
    Json::StreamWriterBuilder swb;
    swb.settings_["emitUTF8"] = true;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    std::stringstream ss;
    int ret = sw->write(val, &ss);
    if (ret != 0)
    {
        LOG_ERROR("Json反序列化失败");
        return false;
    }
    dst = ss.str();
    return true;
}

bool UnSerialize(const std::string &src, Json::Value &val)
{
    Json::CharReaderBuilder crb;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    std::string err;
    bool ret = cr->parse(src.c_str(), src.c_str() + src.size(), &val, &err);
    if (ret == false)
    {
        LOG_ERROR("Json反序列化失败: {}", err);
        return false;
    }
    return true;
}

// ESIndex implementation
ESIndex::ESIndex(std::shared_ptr<elasticlient::Client>& client,
    const std::string& name,
    const std::string& type)
        :_name(name), _type(type), _client(client)
{
    Json::Value analysis;
    Json::Value analyzer;
    Json::Value ik;
    Json::Value tokenizer;
    tokenizer["tokenizer"] = "ik_max_word";
    ik["ik"] = tokenizer;
    analyzer["analyzer"] = ik;
    analysis["analysis"] = analyzer;
    _index["settings"] = analysis;
}

ESIndex& ESIndex::Append(const std::string& key,
    const std::string& type,
    const std::string &analyzer,
    bool enabled)
{
    Json::Value fields;
    fields["type"] = type;
    fields["analyzer"] = analyzer;
    if (enabled == false)
        fields["enabled"] = enabled;
    _properties[key] = fields;
    return *this;
}

bool ESIndex::Create(const std::string& index_id)
{
    Json::Value mappings;
    mappings["dynamic"] = true;
    mappings["properties"] = _properties;
    _index["mappings"] = mappings;

    std::string body;
    bool ret = Serialize(_index, body);
    if (ret == false)
    {
        LOG_ERROR("索引序列化失败!");
        return false;
    }
    LOG_DEBUG("{}", body);
    try
    {
        auto resp = _client->index(_name, _type, index_id, body);
        if (resp.status_code < 200 || resp.status_code >= 300)
        {
            LOG_ERROR("创建ES索引 {} 失败，响应状态码异常: {}", _name, resp.status_code);
            return false;
        }
    }
    catch (std::exception& e)
    {
        LOG_ERROR("创建ES索引 {} 失败，响应状态码异常： {}", _name, e.what());
        return false;
    }
    return true;
}

// ESInsert implementation
ESInsert::ESInsert(const std::shared_ptr<elasticlient::Client>& client,
    const std::string& name,
    const std::string& type)
        :_name(name), _type(type), _client(client)
{
}

bool ESInsert::Insert(const std::string &id)
{
    std::string body;
    bool ret = Serialize(_item, body);
    if (ret == false)
    {
        LOG_ERROR("索引序列化失败！");
        return false;
    }
    LOG_DEBUG("{}", body);
    //2. 发起搜索请求
    try
    {
        auto rsp = _client->index(_name, _type, id, body);
        if (rsp.status_code < 200 || rsp.status_code >= 300)
        {
            LOG_ERROR("新增数据 {} 失败，响应状态码异常: {}", body, rsp.status_code);
            return false;
        }
    } catch(std::exception &e)
    {
        LOG_ERROR("新增数据 {} 失败: {}", body, e.what());
        return false;
    }
    return true;
}

// ESRemove implementation
ESRemove::ESRemove(const std::shared_ptr<elasticlient::Client> &client,
    const std::string &name,
    const std::string &type)
    :_name(name), _type(type), _client(client)
{
}

bool ESRemove::Remove(const std::string &id) {
    try {
        auto rsp = _client->remove(_name, _type, id);
        if (rsp.status_code < 200 || rsp.status_code >= 300) {
            LOG_ERROR("删除数据 {} 失败，响应状态码异常: {}", id, rsp.status_code);
            return false;
        }
    } catch(std::exception &e) {
        LOG_ERROR("删除数据 {} 失败: {}", id, e.what());
        return false;
    }
    return true;
}

// ESSearch implementation
ESSearch::ESSearch(const std::shared_ptr<elasticlient::Client>& client,
    const std::string& name,
    const std::string& type )
    :_name(name), _type(type), _client(client)
{
}

ESSearch& ESSearch::AppendMustNotTerms(const std::string& key, const std::vector<std::string> & vals)
{
    Json::Value fields;
    for (const auto& val : vals)
        fields[key].append(val);
    Json::Value terms;
    terms["terms"] = fields;
    _must_not.append(terms);
    return *this;
}

ESSearch& ESSearch::AppendShouldMatch(const std::string& key, const std::string& val)
{
    Json::Value field;
    field[key] = val;
    Json::Value match;
    match["match"] = field;
    _should.append(match);
    return *this;
}

ESSearch& ESSearch::AppendMustTerm(const std::string& key, const std::string& val)
{
    Json::Value field;
    field[key] = val;
    Json::Value term;
    term["term"] = field;
    _must.append(term);
    return *this;
}

ESSearch &ESSearch::AppendMustMatch(const std::string &key, const std::string &val)
{
    Json::Value field;
    field[key] = val;
    Json::Value match;
    match["match"] = field;
    _must.append(match);
    return *this;
}

Json::Value ESSearch::Search()
{
    Json::Value cond;
    if (_must_not.empty() == false)
        cond["must_not"] = _must_not;
    if (_should.empty() == false)
        cond["should"] = _should;
    if (_must.empty() == false)
        cond["must"] = _must;
    Json::Value query;
    query["bool"] = cond;
    Json::Value root;
    root["query"] = query;

    std::string body;
    bool ret = Serialize(root, body);
    if (ret == false) {
        LOG_ERROR("索引序列化失败！");
        return Json::Value();
    }
    LOG_DEBUG("{}", body);
    //2. 发起搜索请求
    cpr::Response rsp;
    try {
        rsp = _client->search(_name, _type, body);
        if (rsp.status_code < 200 || rsp.status_code >= 300) {
            LOG_ERROR("检索数据 {} 失败，响应状态码异常: {}", body, rsp.status_code);
            return Json::Value();
        }
    } catch(std::exception &e) {
        LOG_ERROR("检索数据 {} 失败: {}", body, e.what());
        return Json::Value();
    }
    //3. 需要对响应正文进行反序列化
    LOG_DEBUG("检索响应正文: [{}]", rsp.text);
    Json::Value json_res;
    ret = UnSerialize(rsp.text, json_res);
    if (ret == false) {
        LOG_ERROR("检索数据 {} 结果反序列化失败", rsp.text);
        return Json::Value();
    }
    return json_res["hits"]["hits"];
}


ESUser::ESUser(const std::shared_ptr<elasticlient::Client>& client)
    :_es_client(client)
{}

bool ESUser::AppendData(const std::string& uid,
    const std::string& phone,
    const std::string& nickname,
    const std::string& description,
    const std::string& avatar_id)
{
    bool ret = ESInsert(_es_client, "user")
        .Append("user_id", uid)
        .Append("nickname", nickname)
        .Append("phone", phone)
        .Append("description", description)
        .Append("avatar_id", avatar_id)
        .Insert(uid);
    if (!ret == false)
    {
        LOG_ERROR("用户数据插入/更新失败!");
        return false;
    }
    LOG_INFO("用户数据新增/更新成功!");
    return true;
}

std::vector<User> ESUser::Search(const std::string& key, const std::vector<std::string>& uid_list)
{
    std::vector<User> res;
    Json::Value json_user = ESSearch(_es_client, "user")
        .AppendShouldMatch("phone.keyword", key)
        .AppendShouldMatch("user_id.keyword", key)
        .AppendShouldMatch("nickname", key)
        .AppendMustNotTerms("user_id.keyword", uid_list)
        .Search();
    if (json_user.isArray() == false) {
        LOG_ERROR("用户搜索结果为空，或者结果不是数组类型");
        return res;
    }
    int sz = json_user.size();
    LOG_DEBUG("检索结果条目数量：{}", sz);
    for (int i = 0; i < sz; i++) {
        User user;
        user.user_id(json_user[i]["_source"]["user_id"].asString());
        user.nickname(json_user[i]["_source"]["nickname"].asString());
        user.description(json_user[i]["_source"]["description"].asString());
        user.phone(json_user[i]["_source"]["phone"].asString());
        user.avatar_id(json_user[i]["_source"]["avatar_id"].asString());
        res.push_back(user);
    }
    return res;
}

ESMessage::ESMessage(const std::shared_ptr<elasticlient::Client>& es_client)
    :_es_client(es_client)
{}

bool ESMessage::CreateIndex()
{
    bool ret = ESIndex(_es_client, "message")
    .Append("user_id", "keyword", "standard", false)
    .Append("message_id", "keyword", "standard", false)
    .Append("create_time", "long", "standard", false)
    .Append("chat_session_id", "keyword", "standard", true)
    .Append("content")
    .Create();
    if (ret == false) {
        LOG_INFO("消息信息索引创建失败!");
        return false;
    }
    LOG_INFO("消息信息索引创建成功!");
    return true;
}


bool ESMessage::AppendData(const std::string &user_id, const std::string &message_id, const long create_time,
                           const std::string &chat_session_id, const std::string &content)
{
    bool ret = ESInsert(_es_client, "message")
    .Append("message_id", message_id)
    .Append("create_time", create_time)
    .Append("user_id", user_id)
    .Append("chat_session_id", chat_session_id)
    .Append("content", content)
    .Insert(message_id);
    if (ret == false) {
        LOG_ERROR("消息数据插入/更新失败!");
        return false;
    }
    LOG_INFO("消息数据新增/更新成功!");
    return true;
}
bool ESMessage::Remove(const std::string &mid)
{
    bool ret = ESRemove(_es_client, "message").Remove(mid);
    if (!ret)
    {
        LOG_ERROR("消息数据删除失败!");
        return false;
    }
    LOG_INFO("消息数据删除成功!");
    return true;
}

std::vector<im::Message> ESMessage::Search(const std::string &key, const std::string &ssid)
{
    std::vector<im::Message> res;
    Json::Value json_user = ESSearch(_es_client, "message")
    .AppendMustTerm("chat_session_id.keyword", ssid)
    .AppendMustMatch("content", key)
    .Search();

    if (json_user.isArray() == false) {
        LOG_ERROR("用户搜索结果为空，或者结果不是数组类型");
        return res;
    }
    int sz = json_user.size();
    LOG_DEBUG("检索结果条目数量：{}", sz);
    for (int i = 0; i < sz; i++) {
        im::Message message;
        message.user_id(json_user[i]["_source"]["user_id"].asString());
        message.message_id(json_user[i]["_source"]["message_id"].asString());
        boost::posix_time::ptime ctime(boost::posix_time::from_time_t(
            json_user[i]["_source"]["create_time"].asInt64()));
        message.create_time(ctime);
        message.session_id(json_user[i]["_source"]["chat_session_id"].asString());
        message.content(json_user[i]["_source"]["content"].asString());
        res.push_back(message);
    }
    return res;
}


} // namespace im
