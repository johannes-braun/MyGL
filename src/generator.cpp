#include "gen/files.hpp"

#include <iostream>

constexpr const char* help_text = R"(
 __  __  _  _  ___  __   
(  \/  )( \/ )/ __)(  )  
 )    (  \  /( (_-. )(__ 
(_/\/\_) (__) \___/(____)

MYGL - OpenGL Function Loader.
-----------------------------------
Options:

-s"path/to/settings.xml"
    Description:    Determines from where the settings.xml should be loaded.
    Default:        "settings.xml"        

-o"path/to/folder"
    Description:    Deposits loader headers into a mygl subdirectory in the given folder.
    Default:        "." (Current executable directory)

-g"path/to/gl.xml"
    Description:    Determines from where the gl.xml should be loaded which is used to generate the loader.
    Default:        "gl.xml"
)";

void print_help() { std::cout << help_text << '\n'; }
int  main(int argc, const char** argv)
{
    std::filesystem::path settings_file_path = "settings.xml";
    std::filesystem::path output_folder_path = ".";
    std::filesystem::path gl_xml_path        = "gl.xml";

    std::basic_string_view<const char*> args{argv + 1, static_cast<size_t>(argc - 1)};
    for(const std::string_view arg : args)
    {
        if(arg[0] == '-')
        {
            switch(arg[1])
            {
            case 's':
                settings_file_path = arg.substr(2);
                break;
            case 'o':
                output_folder_path = arg.substr(2);
                break;
            case 'g':
                gl_xml_path = arg.substr(2);
                break;
            case 'h':
                print_help();
                return 0;
            default:
                std::cerr << "Unknown option: -" << arg[1] << ". Use -h to show the help screen.\n";
                break;
            }
        }
        else
        {
            std::cerr << "Malformed Argument list. Options should start with a dash (-).";
            break;
        }
    }

    std::cout << "-- Settings from " << settings_file_path << "\n";
    std::cout << "-- OpenGL XML from " << gl_xml_path << "\n";
    std::cout << "-- Outputting to " << output_folder_path << "/mygl\n";

    const gen::settings s(settings_file_path, gl_xml_path);

    create_directories(output_folder_path / "mygl");

    std::cout << "---- Writing type infos...\n";
    const files::typeinfos infos = files::write_types(s, output_folder_path);
    std::cout << "---- Writing enumerations...\n";
    files::write_enums(s, output_folder_path);
    std::cout << "---- Writing extensions...\n";
    files::write_extensions(s, output_folder_path);
    std::cout << "---- Writing commands...\n";
    files::write_commands(s, output_folder_path, infos);
    std::cout << "---- Writing loader...\n";
    files::write_loader(s, output_folder_path);
    std::cout << "---- Writing interface...\n";
    files::write_interface(s, output_folder_path);
    std::cout << "-- OpenGL loader generation finished\n";

    return 0;
}