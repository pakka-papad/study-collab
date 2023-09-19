#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <set>

class Group {
    public:
    std::string groupId;
    std::string groupName;
    std::string createdBy;
    std::set<std::string> members;
};

#endif