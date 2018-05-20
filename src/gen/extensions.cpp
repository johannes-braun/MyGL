#include "files.hpp"
#include <fstream>

namespace files
{
constexpr const char* file_extensions_info = R"(#pragma once
/* This header contains all loaded extension definitions.
*/
)";

void write_extensions(const gen::settings& settings, const std::filesystem::path& install_dir)
{
    std::ofstream file_extensions(install_dir / "mygl/mygl_extensions.hpp");
    file_extensions << file_extensions_info;
    for(auto&& ext : settings.enabled_extensions)
    {
        file_extensions << "#ifndef " << ext << "\n";
        file_extensions << "#define " << ext << " " << 1 << "\n";
        file_extensions << "#endif //" << ext << " " << "\n\n";
    }
}
}