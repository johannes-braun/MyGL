cmake_minimum_required(VERSION 3.8)
project(MyGL CXX)

option(MYGL_INSTALL "Generate installation target" ON)

if(NOT MYGL_SETTINGS_PATH)
    set(MYGL_SETTINGS_PATH ${PROJECT_SOURCE_DIR}/res/settings.xml)
endif()
add_definitions(-DMYGL_SETTINGS_PATH="${MYGL_SETTINGS_PATH}")

set_property(GLOBAL PROPERTY CXX_STANDARD 17)

set(EXT_EXTERNAL_LIBRARIES "${PROJECT_SOURCE_DIR}/external")
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin")
set(EXE_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(GLOB_RECURSE SRC_CPP "${SOURCE_DIR}/*.cpp")
add_executable(mygl_gen ${SRC_CPP})
#add_dependencies(mygl_gen ${DEPENDENCIES})
#target_link_libraries(mygl_gen ${LIBRARIES})
set_target_properties(mygl_gen PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(mygl_gen PROPERTIES CXX_STANDARD 17)
#target_include_directories(mygl_gen PUBLIC ${INCLUDES})
add_custom_command(TARGET mygl_gen POST_BUILD COMMAND ${EXE_PATH}/mygl_gen ${PROJECT_SOURCE_DIR}/include WORKING_DIRECTORY ${EXE_PATH})

file(DOWNLOAD https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/master/xml/gl.xml ${PROJECT_SOURCE_DIR}/res/gl.xml)
set(MYGL_GL_XML_PATH "${PROJECT_SOURCE_DIR}/res/gl.xml")
add_definitions(-DMYGL_GL_XML_PATH="${MYGL_GL_XML_PATH}")

if(MYGL_INSTALL)
    install(CODE "execute_process(COMMAND ${EXE_PATH}/mygl_gen ${PROJECT_SOURCE_DIR}/include WORKING_DIRECTORY ${EXE_PATH})")

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/mygl DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/mygl DESTINATION include
        FILES_MATCHING PATTERN "*.inl")
endif()