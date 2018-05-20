#include "files.hpp"

#include <fstream>
#include <sstream>

namespace files
{
constexpr const char* file_types_info = R"(#pragma once
/* This header contains the needed basic types for OpenGL.
INFO: the GLenum type is defined externally in mygl_enums.hpp.
*/

#include "mygl_enums.hpp"
#include <cinttypes>
)";

typeinfos write_types(const gen::settings& settings, const std::filesystem::path& install_dir)
{
    typeinfos infos;

    std::ofstream file_types(absolute(install_dir) / "mygl/mygl_types.hpp");
    file_types << file_types_info;

    for(pugi::xml_node type : settings.opengl_xml.child("registry").child("types").children("type"))
    {
        std::string val = type.first_child().value();
        if(memcmp(val.data(), "typedef", 7) == 0)
        {
            if(val.back() == '*' || val.find_first_of('(') != std::string::npos)
            {
                std::stringstream strm;
                for(pugi::xml_node tc : type.children())
                {
                    if(std::strcmp(tc.name(), "apientry") == 0)
                        strm << "";
                    else if(std::strcmp(tc.name(), "name") == 0)
                        strm << tc.first_child().value();
                    else
                        strm << tc.value();
                }
                strm << "\n";
                std::string str = strm.str();

                size_t it = 0;
                for(auto&& td : infos.typedefs)
                    while((it = str.find(td.first)) != std::string::npos)
                        str.replace(
                                str.begin() + it, str.begin() + it + td.first.size(), td.second);
                file_types << str;
            }
            else
            {
                if(!settings.use_khr &&
                   strcmp("khrplatform", type.attribute("requires").as_string("")) == 0)
                    continue;
                if(strcmp(type.child("name").first_child().value(), "GLenum") == 0)
                    continue;
                if(strcmp(type.child("name").first_child().value(), "GLboolean") == 0)
                    continue;
                val.replace(val.begin(), val.begin() + 8, "");
                val.replace(val.end() - 1, val.end(), "");
                const char* name = type.child("name").first_child().value();

                if(infos.typedefs.count(val) != 0)
                    infos.typedefs.emplace(name, infos.typedefs[val]);
                else
                    infos.typedefs.emplace(name, val);
            }
        }
    }
    infos.typedefs["GLboolean"] = "bool";
    infos.typedefs.erase("GLbitfield");

    for(const auto& pair : infos.typedefs)
    {
        file_types << "using " << pair.first << " = " << pair.second << ";\n";
    }

    pugi::xml_node rules           = settings.settings_xml.child("mygl-generator").child("rules");
    const char*    rules_namespace = rules.attribute("namespace").as_string(nullptr);

    file_types << "\n//custom rule types:\n";
    if(rules_namespace)
        file_types << "namespace " << rules_namespace << "{\n";

    const char* indent = rules_namespace ? "    " : "";
    const std::string namesp = rules_namespace ? rules_namespace + std::string("::") : "";
    for(pugi::xml_node rule : rules)
    {
        if(std::strcmp(rule.name(), "handle-rule") == 0)
        {
            const std::string tname = rule.attribute("typename").as_string();
            const std::string type  = rule.attribute("type").as_string();

            file_types << indent << "enum class " << tname << " : " << type << " { zero = 0 };\n";

            for(pugi::xml_node matcher_rule : rule)
            {
                if(std::strcmp("match", matcher_rule.name()) == 0)
                {
                    matcher m;
                    m.command_expression =
                            matcher_rule.attribute("command-expression").as_string(".*");
                    m.param_expression = matcher_rule.attribute("param-expression").as_string("a^");
                    m.enable_return    = matcher_rule.attribute("enable-return").as_bool(false);
                    m.tname            = namesp + tname;
                    infos.type_matchers.push_back(m);
                }
            }
        }
    }
    if(rules_namespace)
        file_types << "}\n";
    return infos;
}
}
