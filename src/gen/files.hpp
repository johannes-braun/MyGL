#pragma once

#include "settings.hpp"
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace files
{
constexpr const char* file_header = "#pragma once\n\n";

struct matcher
{
    std::regex  param_expression{"a^"};
    std::regex  command_expression{".*"};
    std::string tname;
    bool        enable_return = false;
};
using typedef_map = std::unordered_map<std::string, std::string>;
struct typeinfos
{
    typedef_map          typedefs;
    std::vector<matcher> type_matchers;
};

typeinfos write_types(const gen::settings& settings, const std::filesystem::path& install_dir);
void      write_enums(const gen::settings& settings, const std::filesystem::path& install_dir);
void      write_extensions(const gen::settings& settings, const std::filesystem::path& install_dir);
void      write_commands(const gen::settings& settings, const std::filesystem::path& install_dir,
                         const typeinfos& infos);
void      write_loader(const gen::settings& settings, const std::filesystem::path& install_dir);
void      write_interface(const gen::settings& settings, const std::filesystem::path& install_dir);
}
