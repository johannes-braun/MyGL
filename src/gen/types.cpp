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
#include <type_traits>
)";

constexpr const char* handle_type_template = R"(    template<typename Ident>
    struct basic_handle
    {
        using identifier_type = Ident;
        using handle_type = std::underlying_type_t<identifier_type>;
        using value_type = handle_type;

        constexpr static basic_handle zero() noexcept;
        constexpr static basic_handle from(handle_type h) noexcept;

        constexpr operator handle_type() const noexcept { return handle; }
        constexpr operator bool() const noexcept { return handle != 0; }
        constexpr bool operator ==(basic_handle other) const noexcept { return handle == other.handle; }
        constexpr bool operator !=(basic_handle other) const noexcept { return handle != other.handle; }
        
        handle_type handle;
    };

    template<typename Ident>
    constexpr basic_handle<Ident> basic_handle<Ident>::zero() noexcept
    {
        return from(0);
    }

    template<typename Ident>
    constexpr basic_handle<Ident> basic_handle<Ident>::from(handle_type h) noexcept
    {
        return basic_handle<Ident>{ static_cast<handle_type>(h) };
    }
)";

typeinfos write_types(const gen::settings& settings, const std::filesystem::path& install_dir)
{
    typeinfos infos;

    std::ofstream file_types(absolute(install_dir) / "mygl/mygl_types.hpp");
    file_types << file_types_info;

    gen::typedef_map function_pointer_types;
    for (auto& rep : settings.type_replacements)
        infos.typedefs.emplace(rep.first, rep.second);
    infos.typedefs.emplace("GLboolean", "bool");
    for(pugi::xml_node type : settings.opengl_xml.child("registry").child("types").children("type"))
    {
        std::string val = type.first_child().value();

        if (settings.use_khr && strcmp("khrplatform", type.attribute("name").as_string("")) == 0)
        {
            for (auto& c : val)
                if (c == '<' || c == '>')
                    c = '\"';
            file_types << "#if __has_include(\"KHR/khrplatform.h\")\n";
            file_types << val;
            file_types << "\n#endif\n";
        }
        else if(memcmp(val.data(), "typedef", 7) == 0)
        {
            if(val.find_first_of('(') != std::string::npos)
            {
                std::stringstream strm;
                strm << std::string_view(val.data() + 8, val.size() - 10);

                const char* name = nullptr;
                for (auto it = std::next(type.children().begin()); it != type.children().end(); ++it)
                {
                    if (std::strcmp(it->name(), "name") == 0)
                        name = it->first_child().value();
                    else if(it->value())
                    {
                        std::string valx = it->value();
                        if (!valx.empty() && valx.front() == ')')
                        {
                            strm << " (*) ";
                            // Space commas out a bit
                            for(auto sit = std::next(valx.begin()); sit != std::prev(valx.end()); ++sit)
                            {
                                strm << *sit;
                                if (*sit == ',')
                                    strm << ' ';
                            }
                        }
                    }
                }

                val = strm.str();
                if (function_pointer_types.count(val) != 0)
                    function_pointer_types.emplace(name, function_pointer_types[val]);
                else
                    function_pointer_types.emplace(name, val);
            }
            else
            {
                if(!settings.use_khr &&
                   strcmp("khrplatform", type.attribute("requires").as_string("")) == 0)
                    continue;
                if(strcmp(type.child("name").first_child().value(), "GLenum") == 0 && settings.type_replacements.count("GLenum") == 0)
                    continue;
                if(strcmp(type.child("name").first_child().value(), "GLboolean") == 0 && settings.type_replacements.count("GLboolean") == 0)
                    continue;

                val.replace(val.begin(), val.begin() + 8, "");
                while (val.back() == ' ')
                    val.replace(val.end() - 1, val.end(), "");
                const char* name = type.child("name").first_child().value();

                if(infos.typedefs.count(val) != 0)
                    infos.typedefs.emplace(name, infos.typedefs[val]);
                else
                    infos.typedefs.emplace(name, val);
            }
        }
    }
    if(settings.type_replacements.count("GLbitfield") == 0)
        infos.typedefs.erase("GLbitfield");

#if defined(MYGL_PRINT_TYPEDEFS)
    file_types << "\n//Define a couple of GL types for compatibility.\n";
    for(const auto& pair : infos.typedefs)
    {
        file_types << "using " << pair.first << " = " << pair.second << ";\n";
    }
#endif

    file_types << "\n//All internal function pointer types\n";
    for(auto& pair : function_pointer_types)
    {
        char* it = &pair.second[0];
        while(*it++)
        {
            for(auto& p : infos.typedefs)
                if(memcmp(p.first.c_str(), it, p.first.length()) == 0)
                {
                    const auto rep_begin = std::distance(&pair.second[0], it);
                    pair.second.replace(rep_begin, p.first.length(), p.second);
                    it = &pair.second[rep_begin + p.first.length()];
                    break;
                }
        }

        file_types << "using " << pair.first << " = " << pair.second << ";\n";
    } 

    pugi::xml_node rules           = settings.settings_xml.child("mygl-generator").child("rules");
    const char*    rules_namespace = rules.attribute("namespace").as_string(nullptr);

    file_types << "\n//custom rule types:\n";
    if(rules_namespace)
        file_types << "namespace " << rules_namespace << "{\n";

    const char* indent = rules_namespace ? "    " : "";
    const std::string namesp = rules_namespace ? rules_namespace + std::string("::") : "";

    file_types << handle_type_template;

    for(pugi::xml_node rule : rules)
    {
        if (std::strcmp(rule.name(), "handle-rule") == 0)
        {
            const std::string tname = rule.attribute("typename").as_string();
            const std::string type = rule.attribute("type").as_string();

            file_types << indent << "namespace ident { enum class " << tname << "_identifier : " << type << " {  }; }\n";
            file_types << indent << "using " << tname << " = basic_handle<ident::" << tname << "_identifier>;\n";

        /*    file_types << indent << "struct " << tname << " { \n";
            file_types << indent << "    constexpr static " << type << " zero = 0; \n";
            file_types << indent << "    constexpr " << tname << "() = default; \n";
            file_types << indent << "    constexpr " << tname << '(' << type << " hnd) noexcept : hnd_(hnd) {} \n";
            file_types << indent << "    constexpr " << tname << "(std::nullptr_t) noexcept : hnd_(zero) {} \n";
            file_types << indent << "    constexpr operator " << type << "() const noexcept { return hnd_; } \n";
            file_types << indent << "    constexpr operator bool() const noexcept { return hnd_ != 0; } \n";
            file_types << indent << "    constexpr " << type << " get_handle() const noexcept { return hnd_; } \n";
            file_types << indent << "    constexpr bool operator ==(" << tname << " other) const noexcept { return hnd_ == other.hnd_; } \n";
            file_types << indent << "    constexpr bool operator !=(" << tname << " other) const noexcept { return hnd_ != other.hnd_; } \n";
            file_types << indent << "private:\n";
            file_types << indent << "    " << type << " hnd_; \n";
            file_types << indent << "};\n\n";*/

            
            /*file_types << indent << "struct " << tname << " {\n" << indent << "public:\n";
            file_types << indent << "    constexpr static " << tname << " zero;\n";
            file_types << indent << "    constexpr " << tname << "() = default;\n";
            file_types << indent << "    constexpr " << tname << "(const " << tname << "&) = default;\n";
            file_types << indent << "    constexpr " << tname << "(" << tname << "&&) = default;\n";
            file_types << indent << "    constexpr " << tname << "& operator=(const " << tname << "&) = default;\n";
            file_types << indent << "    constexpr " << tname << "& operator=(" << tname << "&&) = default;\n";
            file_types << indent << "    ~" << tname << "() = default;\n";
            file_types << indent << "    template<typename T, typename = std::enable_if_t<!std::is_same_v<T, "<<tname<<"> && std::is_convertible_v<T, "<<type<<">>>\n";
            file_types << indent << "    explicit constexpr " << tname << "(const T& id) : _id(static_cast<"<<type<<">(id)) {}\n";
            file_types << indent << "    template<typename T, typename = std::enable_if_t<!std::is_same_v<T, "<<tname<<"> && std::is_convertible_v<T, "<<type<<">>>\n";
            file_types << indent << "    explicit constexpr operator T() const noexcept { return static_cast<T>(_id); }\n";
            file_types << indent << "    friend constexpr bool operator ==(const " << tname << "& lhs, const " << tname << "& rhs) noexcept;\n";
            file_types << indent << "    friend constexpr bool operator !=(const " << tname << "& lhs, const " << tname << "& rhs) noexcept;\n";
            file_types << indent << "private:\n    " << indent << type << " _id = 0;\n" << indent << "};\n";
            file_types << indent << "constexpr bool operator ==(const " << tname << "& lhs, const " << tname << "& rhs) noexcept { return lhs._id == rhs._id; }\n";
            file_types << indent << "constexpr bool operator !=(const " << tname << "& lhs, const " << tname << "& rhs) noexcept { return lhs._id != rhs._id; }\n";
            file_types << indent << '\n';*/

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
