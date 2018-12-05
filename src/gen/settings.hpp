#pragma once
#include "../pugixml.hpp"
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

namespace gen
{
using string_set = std::unordered_set<std::string>;
using typedef_map = std::unordered_map<std::string, std::string>;
enum class profile
{
    core,
    compat
};
class settings
{
public:
    settings(const std::filesystem::path& settings_xml_path,
             const std::filesystem::path& opengl_xml_path);

    profile            gl_profile;
    bool               use_khr;
    string_set         enabled_extensions;
    string_set         additional_commands;
    pugi::xml_document settings_xml;
    pugi::xml_document opengl_xml;
    string_set         enums;
    string_set         commands;
    typedef_map        type_replacements;
    int version_major;
    int version_minor;
    struct
    {
        bool keep_prefix;
        bool start_caps;
    } dispatch_options;
};
}
