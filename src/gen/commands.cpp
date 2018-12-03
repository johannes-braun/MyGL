#include "files.hpp"
#include <fstream>

namespace files
{
constexpr const char* file_functions_info = R"(#pragma once 
/* This header contains all needed OpenGL function pointers.
*/
#include "mygl_types.hpp"
#include <memory>

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

    std::stringstream file_context;
    file_context << "\nnamespace mygl {\nstruct dispatch {\n";

    pugi::xml_node commands_node = settings.opengl_xml.child("registry")
                                           .find_child_by_attribute("commands", "namespace", "GL");
    for(auto&& c : commands_node.children("command"))
    {
        auto        proto = c.child("proto");
        std::string pname = proto.child("name").first_child().value();
        auto        it    = settings.commands.begin();
        if((it = settings.commands.find(pname)) == settings.commands.end())
            continue;

        //file_functions << "extern ";
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
                        file_functions << match.tname << " ";
                        file_functions_inl << match.tname << " ";
                        found = true;
                        break;
                    }
                }
                const char* tname = param.first_child().value();
                if(!found)
                {
                    if(const auto iter = infos.typedefs.find(tname); iter != infos.typedefs.end())
                    {
                        file_functions << iter->second << " ";
                        file_functions_inl << iter->second << " ";
                    }
                    else
                    {
                        file_functions << tname << " ";
                        file_functions_inl << tname << " ";
                    }
                }
            }
            else if(std::strcmp(param.name(), "name") == 0)
            {
                file_functions << param.first_child().value() << "(";
                file_functions_inl << param.first_child().value() << "(";
            }
            else
            {
                file_functions << param.value();
                file_functions_inl << param.value();
            }
        }

        bool first = true;

        std::stringstream call;
        call << "return get_current_dispatch()->" << pname << "(";
        for(auto&& p : c.children("param"))
        {
            if(!first)
            {
                file_functions << ", ";
                file_functions_inl << ", ";
                call << ", ";
            }

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
                            file_functions_inl << match.tname;
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                        if(const auto iter = infos.typedefs.find(tname);
                           iter != infos.typedefs.end())
                           {
                            file_functions << iter->second;
                            file_functions_inl << iter->second;
                           }
                        else
                        {
                            file_functions << tname;
                            file_functions_inl << tname;
                        }
                    continue;
                }
                else if(std::strcmp(param.name(), "name") == 0)
                {
                    file_functions << " " << param.first_child().value();
                    file_functions_inl << " " << param.first_child().value();
                    call << param.first_child().value();
                }
                else
                {
                    file_functions << param.value();
                    file_functions_inl << param.value();
                    call << param.value();
                }
            }

            first = false;
        }
        file_functions << ") MYGL_NOEXCEPT;\n";
        file_functions_inl << ") MYGL_NOEXCEPT { " << call.str() << "); }\n";

        file_context << "    decltype(::" << pname << ")* " << pname << " = nullptr;\n";
    }

    file_functions_inl << R"(
namespace mygl {
namespace {
    inline static dispatch  static_dispatch;
    dispatch* current_dispatch = &static_dispatch;
}

dispatch* create_dispatch()
{
    return new dispatch;
}

void destroy_dispatch(dispatch* d)
{
    delete d;
}

dispatch* get_current_dispatch()
{
    return current_dispatch;
}

void set_current_dispatch(dispatch* d)
{
    current_dispatch = d;
}

void dispatch_deleter::operator()(dispatch* d)
{ 
    set_current_dispatch(&static_dispatch); 
    destroy_dispatch(d); 
}

unique_dispatch create_dispatch()
{
    return unique_dispatch{create_dispatch()};
}
}
    )";

    file_context << R"(
    dispatch() = default;
    dispatch(const dispatch&) = delete;
    dispatch(dispatch&&) = default;
    dispatch& operator=(const dispatch&) = delete;
    dispatch& operator=(dispatch&&) = default;
    ~dispatch() = default;
)";

    file_context << "};\n"
        << 
            "dispatch* create_dispatch();\n"
            "void destroy_dispatch(dispatch* d);\n"
            "dispatch* get_current_dispatch();\n"
            "void set_current_dispatch(dispatch* d);\n"

        R"(
struct dispatch_deleter {
    void operator()(dispatch* d) const;
};
using unique_dispatch = std::unique_ptr<dispatch, dispatch_deleter>;
unique_dispatch create_dispatch_unique();
)"
        "} // mygl\n";
    file_functions << file_context.str();
    file_functions
            << "\n#if defined(MYGL_IMPLEMENTATION)\n#include \"mygl_functions.inl\"\n#endif\n";
}
}