#include <array>
#include <numeric>
#include "pugixml.hpp"
#include <fstream>
#include <set>
#include <vector>
#include <cctype>
#include <sstream>
#include <map>
#include <experimental/filesystem>
#include <iostream>
#include <regex>

#define stringize(X) #X

int main(int argc, const char** argv)
{ 
    pugi::xml_document settings;
    settings.load_file(MYGL_SETTINGS_PATH);
     
    pugi::xml_node platform = settings.child("mygl-generator").child("platform");
    pugi::xml_node exts = settings.child("mygl-generator").child("extensions");
    pugi::xml_node cmds = settings.child("mygl-generator").child("commands");
    pugi::xml_node rules = settings.child("mygl-generator").child("rules");;

    bool core = std::strcmp(platform.first_child().value(), "compatibility") != 0;
    bool khrplatform = platform.attribute("use-khrplatform").as_bool(false);
    std::set<std::string> enable_extensions;
    for (pugi::xml_node e : exts.children("extension"))
        enable_extensions.emplace(e.first_child().value());
    std::set<std::string> additional_commands;
    for (pugi::xml_node c : cmds.children("command"))
        additional_commands.emplace(c.first_child().value());

    pugi::xml_document doc;
    doc.load_file(MYGL_GL_XML_PATH);

    std::cout << "Your XML settings file is at " << MYGL_SETTINGS_PATH << '\n';

    constexpr const char* header = "#pragma once\n\n";
    constexpr const char* file_types_info = R"cpp(/*
This header contains the needed basic types for OpenGL.
INFO: the GLenum type is defined externally in gl_enums.hpp.
*/
)cpp";
    constexpr const char* file_enums_info = R"cpp(/*
This header contains all enum values wrapped inside a GLenum enum.
*/
)cpp";
    constexpr const char* file_functions_info = R"cpp(/*
This header contains all needed OpenGL function ptrs.
*/
)cpp";
    constexpr const char* file_loader_info = R"cpp(/*
This header contains the function loader functionalities.
*/
)cpp";
    constexpr const char* file_extensions_info = R"cpp(/*
This header contains all loaded extension definitions.
*/
)cpp";

    std::experimental::filesystem::path install_dir(argc > 1 ? argv[1] : "");

    std::cout << "Generating mygl headers...\n";

    std::experimental::filesystem::create_directories(install_dir / "mygl");
    std::ofstream file_types(install_dir / "mygl/gl_types.hpp");
    std::ofstream file_enums(install_dir / "mygl/gl_enums.hpp");
    std::ofstream file_functions(install_dir / "mygl/gl_functions.hpp");
    std::ofstream file_functions_inl(install_dir / "mygl/gl_functions.inl");
    std::ofstream file_loader(install_dir / "mygl/gl_loader.hpp");
    std::ofstream file_loader_inl(install_dir / "mygl/gl_loader.inl");
    std::ofstream file_extensions(install_dir / "mygl/gl_extensions.hpp");
    std::ofstream file_gl(install_dir / "mygl/gl.hpp");

    file_gl << header << "#include \"gl_types.hpp\"\n#include \"gl_enums.hpp\"\n#include \"gl_functions.hpp\"\n#include \"gl_loader.hpp\"\n#include \"gl_extensions.hpp\"\n";

    file_types << file_types_info << header << "#include <cinttypes>\n#include \"gl_enums.hpp\"\n\n";
    file_enums << file_enums_info << header;
    file_functions << file_functions_info << header << "#include \"gl_types.hpp\"\n\n";
    file_functions_inl << header;
    file_loader << file_loader_info << header;
    file_loader_inl << header;
    file_extensions << file_extensions_info << header;

    std::set<std::string> enums;
    std::set<std::string> commands;

    pugi::xml_node registry = doc.child("registry");
    pugi::xml_node extensions = registry.child("extensions");
    pugi::xml_node types = registry.child("types");

    std::map<std::string, std::string> typedefs;

    for (pugi::xml_node type : types.children("type"))
    {
        std::string val = type.first_child().value();
        if (memcmp(val.data(), "typedef", 7) == 0)
        {
            if (val.back() == '*' || val.find_first_of('(') != std::string::npos)
            {
                std::stringstream strm;
                for (pugi::xml_node tc : type.children())
                {
                    if (std::strcmp(tc.name(), "apientry") == 0)
                        strm << "";
                    else if (std::strcmp(tc.name(), "name") == 0)
                        strm << tc.first_child().value();
                    else
                        strm << tc.value();
                }
                strm << "\n";
                std::string str = strm.str();

                size_t it = 0;
                for (auto&& td : typedefs)
                    while ((it = str.find(td.first)) != std::string::npos)
                        str.replace(str.begin() + it, str.begin() + it + td.first.size(), td.second);
                file_types << str;
            }
            else
            {
                if (!khrplatform && strcmp("khrplatform", type.attribute("requires").as_string("")) == 0)
                    continue;
                if (strcmp(type.child("name").first_child().value(), "GLenum") == 0)
                    continue;
                if (strcmp(type.child("name").first_child().value(), "GLboolean") == 0)
                    continue;
                val.replace(val.begin(), val.begin() + 8, "");
                val.replace(val.end() - 1, val.end(), "");
                const char* name = type.child("name").first_child().value();

                if (typedefs.count(val) != 0)
                    typedefs.emplace(name, typedefs[val]);
                else
                    typedefs.emplace(name, val);
            }
        }
    }
    typedefs["GLboolean"] = "bool";
    typedefs.erase("GLbitfield");

    for (const auto& pair : typedefs)
    {
        file_types << "using " << pair.first << " = " << pair.second << ";\n";
    }

    for (pugi::xml_node feature : registry.children("feature"))
    {
        if (std::strcmp(feature.attribute("api").as_string(), "gl") != 0)
            continue;

        for (pugi::xml_node require : feature.children("require"))
        {
            if (!core || (!require.attribute("profile") || std::strcmp(require.attribute("profile").as_string(), "core") == 0))
            {
                if (auto comment = require.attribute("comment"))
                {
                    std::string comment_str = comment.as_string();
                    if (memcmp("Reuse ", comment_str.data(), 6) == 0)
                    {
                        std::string sub = comment_str.substr(6, comment_str.size() - 6);
                        const char* beg = sub.data();
                        const char* end = sub.data() + sub.size();
                        if (memcmp(beg, "tokens ", 7) == 0)
                            beg += 12;
                        else if (memcmp(beg, "commands ", 9) == 0)
                            beg += 14;
                        if (*(end - 1) == ')')
                        {
                            while (*end != '(')
                                --end;
                            end -= 1;
                        }
                        sub = std::string(beg, end);
                        auto found = sub.find_first_of('-');
                        if (found != std::string::npos)
                        {
                            sub.resize(found - 1);
                        }

                        enable_extensions.emplace("GL_" + sub);
                    }
                }

                for (pugi::xml_node enumerator : require.children("enum"))
                    enums.emplace(enumerator.attribute("name").as_string());
                for (pugi::xml_node command : require.children("command"))
                    commands.emplace(command.attribute("name").as_string());
            }
        }
        for (pugi::xml_node remove : feature.children("remove"))
        {
            if (!core || (!remove.attribute("profile") || std::strcmp(remove.attribute("profile").as_string(), "core") == 0))
            {
                for (pugi::xml_node enumerator : remove.children("enum"))
                    enums.erase(enumerator.attribute("name").as_string());
                for (pugi::xml_node command : remove.children("command"))
                    commands.erase(command.attribute("name").as_string());
            }
        }
    }
    commands.insert(additional_commands.begin(), additional_commands.end());
    for (auto&& ext : enable_extensions)
    {
        file_extensions << "#define " << ext << " " << 1 << "\n";
    }

    for (pugi::xml_node extension : extensions)
    {
        const char* name = extension.attribute("name").as_string();
        std::set<std::string>::iterator it;
        if ((it = enable_extensions.find(name)) != enable_extensions.end())
        {
            for (pugi::xml_node require : extension.children("require"))
            {
                for (pugi::xml_node enumerator : require.children("enum"))
                    enums.emplace(enumerator.attribute("name").as_string());
                for (pugi::xml_node command : require.children("command"))
                    commands.emplace(command.attribute("name").as_string());
            }
        }
    }

    auto commands_node = doc.child("registry").find_child_by_attribute("commands", "namespace", "GL");

    struct matcher
    {
        std::regex param_expression{ "a^" };
        std::regex command_expression{ ".*" };
        std::string tname;
        bool enable_return = false;
    };

    std::vector<matcher> type_matchers;

    for (pugi::xml_node rule : rules)
    {
        if (std::strcmp(rule.name(), "handle-rule") == 0 )  
        {
            const std::string tname = rule.attribute("typename").as_string();
            const std::string type = rule.attribute("type").as_string();

            file_types << "enum class " << tname << " : " << type << " { zero = 0 };\n";

            for (pugi::xml_node matcher_rule : rule)
            {
                if (std::strcmp("match", matcher_rule.name()) == 0)
                {
                    matcher m;
                    m.command_expression = matcher_rule.attribute("command-expression").as_string(".*");
                    m.param_expression = matcher_rule.attribute("param-expression").as_string("a^");
                    m.enable_return = matcher_rule.attribute("enable-return").as_bool(false);
                    m.tname = tname;
                    type_matchers.push_back(m);
                }
            }
        }
    }

    file_enums << R"cpp(enum GLenum {
)cpp";
    for (pugi::xml_node enums_node : registry)
    {
        bool has = false;
        for (pugi::xml_node enums_val : enums_node.children("enum"))
        {
            auto it = enums.begin();
            if ((it = enums.find(enums_val.attribute("name").as_string())) != enums.end())
            {
                file_enums << "    " << *it << " = " << enums_val.attribute("value").as_string() << ",\n";
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


    //for (pugi::xml_node enums_node : registry)
    //{
    //    if (std::strcmp(enums_node.attribute("type").as_string(), "bitmask") == 0)
    //    {
    //        file_enums << "\n\nenum class " << enums_node.attribute("group").as_string() << "{\n";
    //        for (pugi::xml_node enums_val : enums_node.children("enum"))
    //        {
    //            auto it = enums.begin();
    //            if ((it = enums.find(enums_val.attribute("name").as_string())) != enums.end())
    //            {
    //                file_enums << "    " << *it << " = GLenum::" << *it << ",\n";
    //            }
    //        }
    //        file_enums << "};\n";
    //    }
    //}

    std::map<std::string, std::string> type_param{
        {"buffer", "gl_buffer_t"},
        { "buffers", "gl_buffer_t" },
        { "framebuffer", "gl_framebuffer_t" },
        { "framebuffers", "gl_framebuffer_t" },
        { "fbos", "gl_framebuffer_t" },
        { "drawFramebuffer", "gl_framebuffer_t" }, 
        { "readFramebuffer", "gl_framebuffer_t" },
        { "texture", "gl_texture_t" },
        { "textures", "gl_texture_t" },
        { "vaobj", "gl_vertex_array_t" },
        { "array", "gl_vertex_array_t" },
        { "arrays", "gl_vertex_array_t" },
        { "renderbuffer", "gl_renderbuffer_t" },
        { "renderbuffers", "gl_renderbuffer_t" },
        { "sampler", "gl_sampler_t" },
        { "samplers", "gl_sampler_t" },
        { "pipeline", "gl_program_pipeline_t" },
        { "pipelines", "gl_program_pipeline_t" },
        { "program", "gl_shader_program_t" },
        { "shader", "gl_shader_t" },
        { "list", "gl_command_list_nv_t" },
        { "lists", "gl_command_list_nv_t" },
        { "state", "gl_state_nv_t" },
        { "states", "gl_state_nv_t" },
        { "path", "gl_path_nv_t" },
        { "paths", "gl_path_nv_t" },
        { "firstPathName", "gl_path_nv_t" },
    };

    for (auto&& c : commands_node.children("command"))
    {
        auto proto = c.child("proto");
        std::string pname = proto.child("name").first_child().value();
        auto it = commands.begin();
        if ((it = commands.find(pname)) == commands.end())
            continue;

        file_functions << "extern ";
        for (pugi::xml_node param : proto.children())
        {
            if (std::strcmp(param.name(), "ptype") == 0)
            {
                bool found = false;
                std::string pt = proto.child("name").first_child().value();
                for (auto&& match : type_matchers)
                {
                    if (match.enable_return && std::regex_match(pt, match.command_expression))
                    {
                        file_functions << match.tname;
                        found = true; 
                        break;
                    }
                }
                const char* tname = param.first_child().value();
                if (!found)
                {
                    if (typedefs.count(tname) != 0)
                        file_functions << typedefs[tname];
                    else
                        file_functions << tname;
                }
            }
            else if (std::strcmp(param.name(), "name") == 0)
            {
                file_functions << "(*" << param.first_child().value() << ")(";
            }
            else
                file_functions << param.value();
        }

        bool first = true;

        bool is_tf = false;
        if (pname.find("TransformFeedback") != std::string::npos)
            is_tf = true;
        bool is_query = false;
        if (!is_tf && pname.find("Quer") != std::string::npos)
            is_query = true;
        bool is_framebuffer = false;
        if (!is_tf && !is_query && pname.find("Framebuffer") != std::string::npos)
            is_framebuffer = true;

        for (auto&& p : c.children("param"))
        {
            if (!first) file_functions << ", ";

            for (pugi::xml_node param : p.children())
            {
                if (std::strcmp(param.name(), "ptype") == 0)
                {
                    const char* tname = param.first_child().value();

                    bool found = false;
                    std::string pt = p.child("name").first_child().value();
                    for (auto&& match : type_matchers)
                    {
                        if (std::regex_match(pname, match.command_expression) && std::regex_match(pt, match.param_expression))
                        {
                            file_functions << match.tname;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        if (typedefs.count(tname) != 0)
                            file_functions << typedefs[tname];
                        else
                            file_functions << tname;
                    continue;
                }
                else if (std::strcmp(param.name(), "name") == 0)
                {
                    file_functions << " " << param.first_child().value();
                }
                else
                    file_functions << param.value();
            }

            first = false;
        }
#if __cpp_noexcept_function_type >= 201510
        file_functions << ") noexcept;\n";
        file_functions_inl << "decltype(" << pname << ") " << pname << ";\n";
#else
        file_functions << ");\n";
        file_functions_inl << "decltype(" << pname << ") " << pname << ";\n";
#endif
    }

    file_functions << "\n#if defined(MYGL_IMPLEMENTATION)\n#include \"gl_functions.inl\"\n#endif\n";

    file_loader << "#include \"gl_functions.hpp\"\n\n";
    file_loader << "namespace mygl { void load(); }\n";
    file_loader << "\n#if defined(MYGL_IMPLEMENTATION)\n#include \"gl_loader.inl\"\n#endif";

    file_loader_inl << R"cpp(
#include <array>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
namespace {
    class function_loader
    {
    public:
        function_loader()
        {
            for (size_t i = 0; i < std::size(libs); ++i) {
    #ifdef _WIN32
                hnd = LoadLibraryA(libs[i]);
    #else
                hnd = dlopen(libs[i], RTLD_LAZY | RTLD_GLOBAL);
    #endif
                if (hnd != nullptr)
                    break;
            }

    #ifdef __APPLE__
            get_fun = nullptr;
    #elif defined _WIN32
            get_fun = reinterpret_cast<decltype(get_fun)>(get_handle(hnd, "wglGetProcAddress"));
    #else
            get_fun = reinterpret_cast<decltype(get_fun)>(get_handle(hnd, "glXGetProcAddressARB"));
    #endif
        }

        void* get(const char* name)
        {
            void* addr = get_fun ? get_fun(name) : nullptr;
            return addr ? addr : get_handle(hnd, name);
        }

    private:
        void *hnd;

        void* get_handle(void* handle, const char* name)
        {
    #if defined _WIN32
            return static_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
    #else
            return dlsym(handle, name);
    #endif
        }

        void* (*get_fun)(const char*) = nullptr;

    #ifdef __APPLE__
        constexpr static std::array<const char *, 4> libs = {
            "../Frameworks/OpenGL.framework/OpenGL",
            "/Library/Frameworks/OpenGL.framework/OpenGL",
            "/System/Library/Frameworks/OpenGL.framework/OpenGL",
            "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
        };
    #elif defined _WIN32
        constexpr static std::array<const char *, 2> libs = { "opengl32.dll" };
    #else
    #if defined __CYGWIN__
        constexpr static std::array<const char *, 3> libs = {
            "libGL-1.so",
    #else
        constexpr static std::array<const char *, 2> libs = {
    #endif
            "libGL.so.1",
            "libGL.so"
        };
    #endif
    };

    void* __get_gl_func(const char* name)
    {
        static function_loader fl;
        return fl.get(name);
    }
}
)cpp";

    file_loader_inl << "\nnamespace mygl {\nvoid load() {\n";
    for (auto&& command : commands)
    {
        file_loader_inl << "    " << command << " = reinterpret_cast<decltype(" << command << ")>(__get_gl_func(\"" << command << "\"));\n";
    }
    file_loader_inl << "}\n}\n";

    return 0;
}