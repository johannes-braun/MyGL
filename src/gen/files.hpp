#pragma once

#include "settings.hpp"
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace files
{
constexpr const char* file_header = "#pragma once\n\n";
constexpr const char* dispatch_type_name = "dispatch_table";

struct matcher
{
    std::regex  param_expression{"a^"};
    std::regex  command_expression{".*"};
    std::string tname;
    bool        enable_return = false;
};
struct typeinfos
{
    gen::typedef_map          typedefs;
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
