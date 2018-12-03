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
    const auto type_it = settings.type_replacements.find("GLenum");
    const auto bitf_it = settings.type_replacements.find("GLbitfield");
    std::string glenum_name = type_it == settings.type_replacements.end() ? "GLenum" : type_it->second;
    std::string glbitfield_name = bitf_it == settings.type_replacements.end() ? "GLbitfield" : bitf_it->second;

    std::ofstream file_enums(absolute(install_dir) / "mygl/mygl_enums.hpp");
    file_enums << file_enums_info << "enum " << glenum_name << " {\n";
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
    file_enums << "using " << glbitfield_name << " = "<< glenum_name <<";\n";
    file_enums << "namespace { using glenum_type = " << glenum_name << "; }";
    file_enums << R"cpp(
constexpr glenum_type operator|(const glenum_type lhs, const glenum_type rhs)
{
    return glenum_type(unsigned(lhs) | unsigned(rhs));
}
constexpr glenum_type operator&(const glenum_type lhs, const glenum_type rhs)
{
    return glenum_type(unsigned(lhs) & unsigned(rhs));
}
constexpr glenum_type operator^(const glenum_type lhs, const glenum_type rhs)
{
    return glenum_type(unsigned(lhs) ^ unsigned(rhs));
}

#if defined(MYGL_DEFINE_GLENUM_ARITHMETIC)
constexpr glenum_type operator+(const glenum_type lhs, const glenum_type rhs)
{
    return glenum_type(unsigned(lhs) + unsigned(rhs));
}

constexpr glenum_type operator-(const glenum_type lhs, const glenum_type rhs)
{
    return glenum_type(unsigned(lhs) - unsigned(rhs));
}
#endif // MYGL_DEFINE_GLENUM_ARITHMETIC

)cpp";
}
}