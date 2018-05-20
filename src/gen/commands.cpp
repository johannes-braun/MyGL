#include "files.hpp"
#include <fstream>

namespace files
{
constexpr const char* file_functions_info = R"(#pragma once 
/* This header contains all needed OpenGL function pointers.
*/
#include "mygl_types.hpp"

#if __cpp_noexcept_function_type >= 201510
#   define MYGL_NOEXCEPT noexcept
#else
#   define MYGL_NOEXCEPT
#endif
)";

void write_commands(const gen::settings& settings, const std::filesystem::path& install_dir,
                    const typeinfos& infos)
{
    std::ofstream file_functions(install_dir / "mygl/mygl_functions.hpp");
    std::ofstream file_functions_inl(install_dir / "mygl/mygl_functions.inl");

    file_functions << file_functions_info;
    file_functions_inl << "#pragma once\n";

    pugi::xml_node commands_node = settings.opengl_xml.child("registry")
                                           .find_child_by_attribute("commands", "namespace", "GL");
    for(auto&& c : commands_node.children("command"))
    {
        auto        proto = c.child("proto");
        std::string pname = proto.child("name").first_child().value();
        auto        it    = settings.commands.begin();
        if((it = settings.commands.find(pname)) == settings.commands.end())
            continue;

        file_functions << "extern ";
        for(pugi::xml_node param : proto.children())
        {
            if(std::strcmp(param.name(), "ptype") == 0)
            {
                bool        found = false;
                std::string pt    = proto.child("name").first_child().value();
                for(auto&& match : infos.type_matchers)
                {
                    if(match.enable_return && std::regex_match(pt, match.command_expression))
                    {
                        file_functions << match.tname;
                        found = true;
                        break;
                    }
                }
                const char* tname = param.first_child().value();
                if(!found)
                {
                    if(const auto iter = infos.typedefs.find(tname); iter != infos.typedefs.end())
                        file_functions << iter->second;
                    else
                        file_functions << tname;
                }
            }
            else if(std::strcmp(param.name(), "name") == 0)
            {
                file_functions << "(*" << param.first_child().value() << ")(";
            }
            else
                file_functions << param.value();
        }

        bool first = true;

        for(auto&& p : c.children("param"))
        {
            if(!first)
                file_functions << ", ";

            for(pugi::xml_node param : p.children())
            {
                if(std::strcmp(param.name(), "ptype") == 0)
                {
                    const char* tname = param.first_child().value();

                    bool        found = false;
                    std::string pt    = p.child("name").first_child().value();
                    for(auto&& match : infos.type_matchers)
                    {
                        if(std::regex_match(pname, match.command_expression) &&
                           std::regex_match(pt, match.param_expression))
                        {
                            file_functions << match.tname;
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                        if(const auto iter = infos.typedefs.find(tname);
                           iter != infos.typedefs.end())
                            file_functions << iter->second;
                        else
                            file_functions << tname;
                    continue;
                }
                else if(std::strcmp(param.name(), "name") == 0)
                {
                    file_functions << " " << param.first_child().value();
                }
                else
                    file_functions << param.value();
            }

            first = false;
        }
        file_functions << ") MYGL_NOEXCEPT;\n";
        file_functions_inl << "decltype(" << pname << ") " << pname << ";\n";
    }
    file_functions
            << "\n#if defined(MYGL_IMPLEMENTATION)\n#include \"mygl_functions.inl\"\n#endif\n";
}
}