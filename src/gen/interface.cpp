#include "files.hpp"
#include <fstream>

namespace files
{
constexpr const char* interface_info = R"(#pragma once
#include "mygl_enums.hpp"
#include "mygl_extensions.hpp"
#include "mygl_functions.hpp"
#include "mygl_loader.hpp"
#include "mygl_types.hpp"
)";

void write_interface(const gen::settings& settings, const std::filesystem::path& install_dir)
{
    std::ofstream file_interface(install_dir / "mygl/mygl.hpp");

    file_interface << interface_info;
}
}