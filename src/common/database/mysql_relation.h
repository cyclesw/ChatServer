#pragma once

#include "relation.hxx"

#include <memory>
#include <vector>
namespace im  
{
    class RelationTable;


    class RelationTable 
    {
    public:
        using Ptr = std::shared_ptr<RelationTable>;
        RelationTable(const std::shared_ptr<odb::core::database>& db);

        bool Insert(const std::string& uid, const std::string& pid);

        bool Remove(const std::string& uid, const std::string& pid);

        bool Exists(const std::string& uid, const std::string& pid);

        std::vector<std::string> Friends(const std::string& uid);

    private:
        std::shared_ptr<odb::core::database> _db;
    };
}