#include "files.hpp"
#include <fstream>

namespace files
{
    constexpr const char* interface_info = R"(#pragma once
/*
 __  __  _  _  ___  __   
(  \/  )( \/ )/ __)(  )  
 )    (  \  /( (_-. )(__ 
(_/\/\_) (__) \___/(____)

MYGL - OpenGL Function Loader.
-----------------------------------

This is a header-only library. But for the sake of not cluttering your sources with my windows.h-like includes, 
you need to include mygl exactly in one cpp file as follows:

#define MYGL_IMPLEMENTATION
#include <mygl.hpp>

You can then run the loader after context-creation and after making a context current with the functions
mygl::load();
    - or -
mygl::load(reinterpret_cast<mygl::loader_function>(myGetProcAddress));

-----------------------------------
*/

#include "mygl_enums.hpp"
#include "mygl_extensions.hpp"
#include "mygl_functions.hpp"
#include "mygl_loader.hpp"
#include "mygl_types.hpp"
)";

    void write_interface(const gen::settings& settings, const std::filesystem::path& install_dir)
    {
        std::ofstream file_interface(install_dir / "mygl/mygl.hpp");
        file_interface << interface_info;

        std::ofstream face_interface_static(install_dir / "mygl/mygl.cpp");
        face_interface_static << R"(#define MYGL_IMPLEMENTATION
#include "mygl.hpp"
        )";
    }
}