#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <set>
#include <nlohmann/json.hpp>

class Group {
    public:
    std::string groupId;
    std::string groupName;
    std::string createdBy;
    std::set<std::string> members;

    nlohmann::json toJson() {
        nlohmann::json res;
        res["group_id"] = groupId;
        res["group_name"] = groupName;
        res["created_by"] = createdBy;
        for(auto &member: members){
            res["members"].push_back(member);
        }
        return res;
    }
};

#endif