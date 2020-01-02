#ifndef IYF_LAUNCHER_DATA_HPP
#define IYF_LAUNCHER_DATA_HPP

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

namespace iyf::launcher {
struct EngineVersionInfo {
    EngineVersionInfo() : major(0), minor(0), patch(0), managedByLanucher(false) {}
    EngineVersionInfo(std::string path, int major, int minor, int patch, bool managedByLanucher)
    : path(std::move(path)), major(major), minor(minor), patch(patch), managedByLanucher(managedByLanucher) {}
    
    std::string path;
    int major;
    int minor;
    int patch;
    bool managedByLanucher;
    
    void serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& wr) const;
    bool deserialize(const rapidjson::Value& doc);
    
    friend bool operator==(const EngineVersionInfo& a, const EngineVersionInfo& b);
};

struct ProjectInfo {
    ProjectInfo() {}
    ProjectInfo(std::string path, std::string editorPath) : path(std::move(path)), editorPath(std::move(editorPath)) {}
    
    std::string path;
    std::string editorPath;
    
    void serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& wr) const;
    bool deserialize(const rapidjson::Value& doc);
    
    friend bool operator==(const ProjectInfo& a, const ProjectInfo& b);
};

}

#endif // IYF_LAUNCHER_DATA_HPP
