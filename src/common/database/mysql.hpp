#pragma once

#include <memory>
#include <odb/mysql/connection-factory.hxx>
#include <odb/mysql/database.hxx>

namespace im
{
class ODBFactory
{
public:
    static std::shared_ptr<odb::core::database> Create(
        const std::string& user,
        const std::string& password,
        const std::string& host,
        const std::string& db,
        const std::string& cset,
        int port,
        int conn_pool_count)
    {
        std::unique_ptr<odb::mysql::connection_pool_factory> cpf(
            new odb::mysql::connection_pool_factory(conn_pool_count, 0));
        auto res = std::make_shared<odb::mysql::database>(user, password,
            db, host, port, "", cset, 0, std::move(cpf));
        return res;
    }
};

}

