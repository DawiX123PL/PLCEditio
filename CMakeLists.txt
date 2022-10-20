cmake_minimum_required(VERSION 3.8)



project("PLCEditio")

set(IMGUI
    "./libs/imgui/imconfig.h"
    "./libs/imgui/imgui.cpp"
    "./libs/imgui/imgui.h"
    "./libs/imgui/imgui_demo.cpp"
    "./libs/imgui/imgui_draw.cpp"
    "./libs/imgui/imgui_internal.h"
    "./libs/imgui/imgui_tables.cpp"
    "./libs/imgui/imgui_widgets.cpp"
    "./libs/imgui/imstb_rectpack.h"
    "./libs/imgui/imstb_textedit.h"
    "./libs/imgui/imstb_truetype.h"
    "./libs/imgui/backends/imgui_impl_glfw.cpp"
    "./libs/imgui/backends/imgui_impl_opengl3.cpp"
    )

add_executable(PLCEditio 
            ${IMGUI}
            "./src/main.cpp"
            "./src/app.hpp"
            )

add_subdirectory("libs/glfw")
include_directories("libs/glfw/include")
include_directories("libs/imgui")
include_directories("libs/imgui/backends")

find_package(OpenGL REQUIRED)
target_link_libraries(PLCEditio OpenGL::GL)

target_link_libraries(PLCEditio glfw ${GLFW_LIBRARIES})
target_link_libraries(PLCEditio glfw ${OPENGL_glu_LIBRARY} ${GLFW_LIBRARIES})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)



set_target_properties(PLCEditio
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/out"
)



# set language standard to c++11
if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET PLCEditio PROPERTY CXX_STANDARD 20)
    set_property(TARGET PLCEditio PROPERTY CXX_STANDARD 20)
endif()

