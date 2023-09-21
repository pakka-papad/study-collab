#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <set>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>


class Group {
    public:
    std::string groupId;
    std::string groupName;
    std::string createdBy;
    std::set<std::string> members;
    std::map<std::string,std::ofstream*> openFiles;
    std::vector<std::string> sharedFiles;

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