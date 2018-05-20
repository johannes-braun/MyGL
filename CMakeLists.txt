cmake_minimum_required(VERSION 3.8)
project(MyGL CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin")

set_property(GLOBAL PROPERTY CXX_STANDARD 17)
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(SOURCE_CPP
    ${SOURCE_DIR}/generator.cpp
    ${SOURCE_DIR}/pugixml.cpp
    ${SOURCE_DIR}/gen/commands.cpp
    ${SOURCE_DIR}/gen/enums.cpp
    ${SOURCE_DIR}/gen/extensions.cpp
    ${SOURCE_DIR}/gen/loader.cpp
    ${SOURCE_DIR}/gen/settings.cpp
    ${SOURCE_DIR}/gen/types.cpp
)
add_executable(mygl_gen ${SOURCE_CPP})
set_target_properties(mygl_gen PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(mygl_gen PROPERTIES CXX_STANDARD 17)
if(UNIX)
    target_link_libraries(mygl_gen stdc++fs)
endif()

file(DOWNLOAD https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/master/xml/gl.xml ${PROJECT_SOURCE_DIR}/bin/gl.xml)
file(COPY ${PROJECT_SOURCE_DIR}/res/settings.xml DESTINATION ${PROJECT_SOURCE_DIR}/bin/)
