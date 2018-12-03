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

If you want to use MyGL as a header-only library, make sure to include it in exactly one cpp with a previous definition of MYGL_IMPLEMENTATION as follows:

    #define MYGL_IMPLEMENTATION
    #include <mygl.hpp>

There is also a pre-generated cpp file so you can also just put the generated files into a source-subdirectory of yours.

You can then run the loader after context-creation and after making a context current with the functions
    
    mygl::load();
        - or -
    mygl::load(reinterpret_cast<mygl::loader_function>(myGetProcAddress));

-----------------------------------

DISPATCH BASED MULTITHREADING

Along with the default loader functionality, you can also create context-based dispatch-table objects:
    
    mygl::dispatch* my_table = mygl::create_dispatch();
    mygl::set_current_dispatch(my_table);
     // ...
    mygl::destroy_dispatch(my_table);

After destroying a dispatch table, the default one is being bound automatically. You can manually reset by passing nullptr to mygl::set_current_dispatch.

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