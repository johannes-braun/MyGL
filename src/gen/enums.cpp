#include "files.hpp"
#include <fstream>

namespace files
{
constexpr const char* file_enums_info = R"(#pragma once
/* This header contains all enum values wrapped inside a GLenum enum.
*/
)";

void write_enums(const gen::settings& settings, const std::filesystem::path& install_dir)
{
    std::ofstream file_enums(absolute(install_dir) / "mygl/mygl_enums.hpp");
    file_enums << file_enums_info << "enum GLenum {\n";
    for(pugi::xml_node enums_node : settings.opengl_xml.child("registry"))
    {
        bool has = false;
        for(pugi::xml_node enums_val : enums_node.children("enum"))
        {
            auto it = settings.enums.begin();
            if((it = settings.enums.find(enums_val.attribute("name").as_string())) !=
               settings.enums.end())
            {
                file_enums << "    " << *it << " = " << enums_val.attribute("value").as_string()
                           << ",\n";
            }
        }
    }
    file_enums << R"cpp(};
)cpp";
    file_enums << "using GLbitfield = GLenum;";
    file_enums << R"cpp(
constexpr GLenum operator|(const GLenum lhs, const GLenum rhs)
{
    return GLenum(unsigned(lhs) | unsigned(rhs));
}

constexpr GLenum operator+(const GLenum lhs, const GLenum rhs)
{
    return GLenum(unsigned(lhs) + unsigned(rhs));
}

constexpr GLenum operator-(const GLenum lhs, const GLenum rhs)
{
    return GLenum(unsigned(lhs) - unsigned(rhs));
})cpp";
}
}