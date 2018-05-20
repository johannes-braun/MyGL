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

void* __get_mygl_func(const char* name)
{
    static function_loader fl;
    return fl.get(name);
}
}
)";

void write_loader(const gen::settings& settings, const std::filesystem::path& install_dir)
{
    std::ofstream file_loader(install_dir / "mygl/mygl_loader.hpp");
    std::ofstream file_loader_inl(install_dir / "mygl/mygl_loader.inl");

    file_loader << file_loader_info;
    file_loader_inl << file_loader_code;

    file_loader_inl << "\nnamespace mygl {\nvoid load() {\n";
    for(auto&& command : settings.commands)
    {
        file_loader_inl << "    " << command << " = reinterpret_cast<decltype(" << command
                        << ")>(__get_mygl_func(\"" << command << "\"));\n";
    }
    file_loader_inl << "}\n}\n";
}
}