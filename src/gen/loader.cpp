#include "files.hpp"
#include <fstream>

namespace files
{
constexpr const char* file_loader_info = R"(#pragma once
/* This header contains the function loader functionalities.
*/
#include "mygl_functions.hpp"

namespace mygl { 
void load();
void load(loader_function fun);
void load(MYGL_DISPATCH_NAME* d);
void load(MYGL_DISPATCH_NAME* d, loader_function fun);
}

#if defined(MYGL_IMPLEMENTATION)
    #include "mygl_loader.inl"
#endif
)";

constexpr const char* file_loader_code = R"(#pragma once
#include <array>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#define MYGL_REMOVE_APIENTRY
#endif // APIENTRY

namespace mygl
{

namespace {
    class function_loader
    {
    public:
        function_loader(loader_function fun)
            : get_fun(static_cast<decltype(get_fun)>(fun))
        {

        }

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

        void* (APIENTRY *get_fun)(const char*) = nullptr;

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
)";

constexpr const char* loading_functions = R"(

void load(MYGL_DISPATCH_NAME* d)
{
    function_loader loader;
    load_impl(d, loader);
}

void load(MYGL_DISPATCH_NAME* d, loader_function fun)
{
    function_loader loader(fun);
    load_impl(d, loader);
}

void load()
{
    load(&get_static_dispatch());
}

void load(loader_function fun)
{
    load(&get_static_dispatch(), fun);
}
)";

void write_loader(const gen::settings& settings, const std::filesystem::path& install_dir)
{
    std::ofstream file_loader(install_dir / "mygl/mygl_loader.hpp");
    std::ofstream file_loader_inl(install_dir / "mygl/mygl_loader.inl");

    file_loader << "#define MYGL_DISPATCH_NAME " << dispatch_type_name << "\n";
    file_loader << file_loader_info;
    file_loader << "#undef MYGL_DISPATCH_NAME\n";
    file_loader_inl << file_loader_code;

    file_loader_inl << "\n    void load_impl(MYGL_DISPATCH_NAME* d, function_loader& loader) {\n";
    for(auto&& command : settings.commands)
    {
        std::string fun_name = command;
        if (!settings.dispatch_options.keep_prefix)
            fun_name.erase(0, 2);
        if (!settings.dispatch_options.start_caps)
            fun_name[0] = std::tolower(fun_name[0], std::locale{});

        file_loader_inl << "        d->" << fun_name << " = reinterpret_cast<decltype(::" << command
                        << ")*>(loader.get(\"" << command << "\"));\n";
    }
    file_loader_inl << "    }\n}";

    file_loader_inl << loading_functions;

    file_loader_inl << "}\n";
    file_loader_inl << R"(#if defined(MYGL_REMOVE_APIENTRY)
#undef APIENTRY
#undef MYGL_REMOVE_APIENTRY
#endif // MYGL_REMOVE_APIENTRY

    )";

}
}