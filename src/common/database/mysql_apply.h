#pragma once

#include <memory>
#include <string>
#include <vector>
#include "friend_apply.hxx"

namespace im
{
    class FriendApplyTable 
    {
    public:
        using Ptr = std::shared_ptr<FriendApplyTable>;
        FriendApplyTable(const std::shared_ptr<odb::core::database>& db);

        bool Insert(FriendApply& apply);
        bool Insert(FriendApply&& apply);

        bool Exists(const std::string& uid, const std::string& pid);

        bool Remove(const std::string& uid, const std::string& pid);

        /*!
         *
         * @param uid 当前用户ID
         * @return 当前用户的好友申请列表
         */
        std::vector<std::string> ApplyUsers(const std::string& uid);

    private:
        std::shared_ptr<odb::core::database> _db;
    }; 
}