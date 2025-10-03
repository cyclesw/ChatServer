#pragma once

#include "log.hpp"

#include <elasticlient/client.h>
#include <json/json.h>

namespace im
{
bool Serialize(const Json::Value& val, std::string& dst);
bool UnSerialize(const std::string &src, Json::Value &val);

// ESIndex类用于创建和管理Elasticsearch索引
// 可以定义索引的映射结构，包括字段类型、分析器等
class ESIndex
{
public:
    ESIndex(std::shared_ptr<elasticlient::Client>& client,
        const std::string& name,
        const std::string& type = "_doc");

    // 添加字段定义到索引映射中
    // 参数：
    // - key: 字段名
    // - type: 字段类型（默认为text）
    // - analyzer: 分析器（默认为ik_max_word）
    // - enabled: 是否启用（默认为true）
    ESIndex& Append(const std::string& key,
        const std::string& type = "text",
        const std::string &analyzer = "ik_max_word",
        bool enabled = true);

    // 创建索引
    // 参数：
    // - index_id: 索引ID（默认为"default_index_id"）
    // 返回值：创建成功返回true，否则返回false
    bool Create(const std::string& index_id = "default_index_id");

private:
    std::string _name;        // 索引名称
    std::string _type;        // 索引类型
    Json::Value _properties;  // 索引属性定义
    Json::Value _index;       // 索引配置
    std::shared_ptr<elasticlient::Client> _client; // Elasticsearch客户端
};

// ESInsert类用于向Elasticsearch索引中插入数据
// 支持链式调用，可以连续添加多个字段
class ESInsert
{
public:
    ESInsert(const std::shared_ptr<elasticlient::Client>& client,
        const std::string& name,
        const std::string& type = "_doc");

    // 添加字段值
    // 参数：
    // - key: 字段名
    // - val: 字段值（模板类型，支持各种数据类型）
    // 返回值：返回当前对象的引用，支持链式调用
    template<class T>
    ESInsert &Append(const std::string &key, const T &val)
    {
        _item[key] = val;
        return *this;
    }

    // 执行插入操作
    // 参数：
    // - id: 文档ID（可选，如果不指定则自动生成）
    // 返回值：插入成功返回true，否则返回false
    bool Insert(const std::string &id = "");

private:
    std::string _name;        // 索引名称
    std::string _type;        // 索引类型
    Json::Value _item;        // 要插入的文档数据
    std::shared_ptr<elasticlient::Client> _client; // Elasticsearch客户端
};

// ESRemove类用于从Elasticsearch索引中删除文档
class ESRemove
{
public:
    ESRemove(const std::shared_ptr<elasticlient::Client> &client,
        const std::string &name,
        const std::string &type = "_doc");

    // 删除指定ID的文档
    // 参数：
    // - id: 要删除的文档ID
    // 返回值：删除成功返回true，否则返回false
    bool Remove(const std::string &id);

private:
    std::string _name;        // 索引名称
    std::string _type;        // 索引类型
    std::shared_ptr<elasticlient::Client> _client; // Elasticsearch客户端
};

// ESSearch类用于在Elasticsearch索引中搜索文档
// 支持多种查询条件组合，包括must、should、must_not等
class ESSearch
{
public:
    ESSearch(const std::shared_ptr<elasticlient::Client>& client,
        const std::string& name,
        const std::string& type = "_doc");

    // 添加must_not查询条件（必须不包含指定值）
    // 参数：
    // - key: 字段名
    // - vals: 不包含的值列表
    // 返回值：返回当前对象的引用，支持链式调用
    ESSearch& AppendMustNotTerms(const std::string& key, const std::vector<std::string> & vals);

    // 添加should查询条件（应该匹配指定值）
    // 参数：
    // - key: 字段名
    // - val: 匹配的值
    // 返回值：返回当前对象的引用，支持链式调用
    ESSearch& AppendShouldMatch(const std::string& key, const std::string& val);

    // 添加must查询条件（必须包含指定词条）
    // 参数：
    // - key: 字段名
    // - val: 要匹配的词条
    // 返回值：返回当前对象的引用，支持链式调用
    ESSearch& AppendMustTerm(const std::string& key, const std::string& val);

    // 添加must查询条件（必须匹配指定文本）
    // 参数：
    // - key: 字段名
    // - val: 要匹配的文本
    // 返回值：返回当前对象的引用，支持链式调用
    ESSearch& AppendMustMatch(const std::string& key, const std::string& val);

    // 执行搜索操作
    // 返回值：返回搜索结果的JSON格式数据
    Json::Value Search();

private:
    std::string _name;        // 索引名称
    std::string _type;        // 索引类型
    Json::Value _must_not;    // must_not查询条件
    Json::Value _should;      // should查询条件
    Json::Value _must;        // must查询条件
    std::shared_ptr<elasticlient::Client> _client; // Elasticsearch客户端
};
} // namespace im
 // namespace im