#include "settings.hpp"
#include <cstring>

namespace gen
{
settings::settings(const std::filesystem::path& settings_xml_path,
                   const std::filesystem::path& opengl_xml_path)
{
    using namespace std::string_view_literals;

    settings_xml.load_file(settings_xml_path.c_str());
    pugi::xml_node gen      = settings_xml.child("mygl-generator");
    pugi::xml_node exts     = gen.child("extensions");
    pugi::xml_node cmds     = gen.child("commands");
    pugi::xml_node platform = gen.child("platform");

    if(platform.first_child().value() == "compat"sv)
        gl_profile = profile::compat;
    else
        gl_profile = profile::core;

    use_khr = platform.attribute("use-khr").as_bool(false);

    for(pugi::xml_node e : exts.children("extension"))
        enabled_extensions.emplace(e.first_child().value());
    for(pugi::xml_node c : cmds.children("command"))
        additional_commands.emplace(c.first_child().value());

    opengl_xml.load_file(opengl_xml_path.c_str());

    for(pugi::xml_node feature : opengl_xml.child("registry").children("feature"))
    {
        if(std::strcmp(feature.attribute("api").as_string(), "gl") != 0)
            continue;

        for(pugi::xml_node require : feature.children("require"))
        {
            if((gl_profile != profile::core) ||
               (!require.attribute("profile") ||
                         std::strcmp(require.attribute("profile").as_string(), "core") == 0))
            {
                if(auto comment = require.attribute("comment"))
                {
                    std::string comment_str = comment.as_string();
                    if(memcmp("Reuse ", comment_str.data(), 6) == 0)
                    {
                        std::string sub = comment_str.substr(6, comment_str.size() - 6);
                        const char* beg = sub.data();
                        const char* end = sub.data() + sub.size();
                        if(memcmp(beg, "tokens ", 7) == 0)
                            beg += 12;
                        else if(memcmp(beg, "commands ", 9) == 0)
                            beg += 14;
                        if(*(end - 1) == ')')
                        {
                            while(*end != '(')
                                --end;
                            end -= 1;
                        }
                        sub        = std::string(beg, end);
                        auto found = sub.find_first_of('-');
                        if(found != std::string::npos)
                        {
                            sub.resize(found - 1);
                        }

                        enabled_extensions.emplace("GL_" + sub);
                    }
                }

                for(pugi::xml_node enumerator : require.children("enum"))
                    enums.emplace(enumerator.attribute("name").as_string());
                for(pugi::xml_node command : require.children("command"))
                    commands.emplace(command.attribute("name").as_string());
            }
        }
        for(pugi::xml_node remove : feature.children("remove"))
        {
            if((gl_profile != profile::core) || (!remove.attribute("profile") ||
                         std::strcmp(remove.attribute("profile").as_string(), "core") == 0))
            {
                for(pugi::xml_node enumerator : remove.children("enum"))
                    enums.erase(enumerator.attribute("name").as_string());
                for(pugi::xml_node command : remove.children("command"))
                    commands.erase(command.attribute("name").as_string());
            }
        }
    }
    commands.insert(additional_commands.begin(), additional_commands.end());
    for(pugi::xml_node extension : opengl_xml.child("registry").child("extensions"))
    {
        const char*                     name = extension.attribute("name").as_string();
        string_set::iterator it;
        if((it = enabled_extensions.find(name)) != enabled_extensions.end())
        {
            for(pugi::xml_node require : extension.children("require"))
            {
                for(pugi::xml_node enumerator : require.children("enum"))
                    enums.emplace(enumerator.attribute("name").as_string());
                for(pugi::xml_node command : require.children("command"))
                    commands.emplace(command.attribute("name").as_string());
            }
        }
    }
}
}