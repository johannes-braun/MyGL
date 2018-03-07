cmake_minimum_required(VERSION 3.8)
project(MyGL CXX)

option(MYGL_INSTALL "Generate installation target" ON)

if(NOT MYGL_SETTINGS_PATH)
    set(MYGL_SETTINGS_PATH ${PROJECT_SOURCE_DIR}/res/settings.xml)
endif()
add_definitions(-DMYGL_SETTINGS_PATH="${MYGL_SETTINGS_PATH}")

set_property(GLOBAL PROPERTY CXX_STANDARD 17)

set(EXT_EXTERNAL_LIBRARIES "${PROJECT_SOURCE_DIR}/external")
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
set(EXT_LIB_DEFAULTS -DCMAKE_VERBOSE_MAKEFILE=OFF
    -DCMAKE_INSTALL_MESSAGE=NEVER 
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>)
macro(load_git_library lib_name lib_repo lib_file git_tag)
    file(REMOVE_RECURSE ${EXT_EXTERNAL_LIBRARIES}/cached/${lib_name})
    ExternalProject_Add(${lib_name}-external
        GIT_REPOSITORY ${lib_repo}
        GIT_TAG ${git_tag}
        UPDATE_COMMAND "" 
        INSTALL_DIR ${EXT_EXTERNAL_LIBRARIES}/cached/${lib_name}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${lib_file}
        BUILD_ALWAYS 0
        LOG_DOWNLOAD 0
        LOG_UPDATE 0
        LOG_DOWNLOAD LOG_DOWNLOAD  0
        LOG_BUILD 0
        LOG_TEST 0
        LOG_INSTALL 0
        CMAKE_ARGS ${EXT_LIB_DEFAULTS} ${EXT_LIB_ARGS})
    add_library(lib${lib_name} STATIC IMPORTED)
    add_dependencies(lib${lib_name} ${lib_name}-external)
    set_target_properties(lib${lib_name} PROPERTIES IMPORTED_LOCATION ${EXT_EXTERNAL_LIBRARIES}/cached/${lib_name}/lib/${lib_file})

    set(EXT_LIB_ARGS "")
    list(APPEND DEPENDENCIES ${lib_name}-external)
    list(APPEND INCLUDES ${EXT_EXTERNAL_LIBRARIES}/cached/${lib_name}/include)
    list(APPEND LIBRARIES lib${lib_name})
endmacro()

load_git_library(pugixml "https://github.com/zeux/pugixml.git" pugixml.lib master)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin")
file(GLOB_RECURSE SRC_CPP "${SOURCE_DIR}/*.cpp")
add_executable(mygl_gen ${SRC_CPP})
add_dependencies(mygl_gen ${DEPENDENCIES})
target_link_libraries(mygl_gen ${LIBRARIES})
set_target_properties(mygl_gen PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(mygl_gen PROPERTIES CXX_STANDARD 17)
target_include_directories(mygl_gen PUBLIC ${INCLUDES})
add_custom_command(TARGET mygl_gen POST_BUILD COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mygl_gen ${PROJECT_SOURCE_DIR}/include WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

file(DOWNLOAD https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/master/xml/gl.xml ${PROJECT_SOURCE_DIR}/res/gl.xml)
set(MYGL_GL_XML_PATH "${PROJECT_SOURCE_DIR}/res/gl.xml")
add_definitions(-DMYGL_GL_XML_PATH="${MYGL_GL_XML_PATH}")

if(MYGL_INSTALL)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/mygl DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")
endif()