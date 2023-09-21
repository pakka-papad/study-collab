#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <set>
#include <nlohmann/json.hpp>
#include <filesystem>


class Group {
    public:
    std::string groupId;
    std::string groupName;
    std::string createdBy;
    std::set<std::string> members;
    std::map<std::string,FILE*> openFiles;
    std::set<std::string> sharedFiles;

    nlohmann::json toJson() {
        nlohmann::json res;
        res["group_id"] = groupId;
        res["group_name"] = groupName;
        res["created_by"] = createdBy;
        for(auto &member: members){
            res["members"].push_back(member);
        }
        for(auto &file: sharedFiles){
            res["files"].push_back(file);
        }
        return res;
    }
};

#endif