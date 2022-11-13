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
    "./libs/imgui/misc/cpp/imgui_stdlib.cpp"
    "./libs/imgui/misc/cpp/imgui_stdlib.h"
    )

set(IMGUI_NODE_EDITOR
    "./libs/imgui-node-editor/crude_json.cpp"
    "./libs/imgui-node-editor/crude_json.h"
    "./libs/imgui-node-editor/imgui_bezier_math.h"
    "./libs/imgui-node-editor/imgui_bezier_math.inl"
    "./libs/imgui-node-editor/imgui_canvas.cpp"
    "./libs/imgui-node-editor/imgui_canvas.h"
    "./libs/imgui-node-editor/imgui_extra_math.h"
    "./libs/imgui-node-editor/imgui_extra_math.inl"
    "./libs/imgui-node-editor/imgui_node_editor.cpp"
    "./libs/imgui-node-editor/imgui_node_editor.h"
    "./libs/imgui-node-editor/imgui_node_editor_api.cpp"
    "./libs/imgui-node-editor/imgui_node_editor_internal.h"
    "./libs/imgui-node-editor/imgui_node_editor_internal.inl"
    )


set(IMNODES
    "libs/imnodes/imnodes_internal.h"
    "libs/imnodes/imnodes.cpp"
    "libs/imnodes/imnodes.h"
    )


add_executable(PLCEditio 
            ${IMGUI} 
            # ${IMGUI_NODE_EDITOR}
            ${IMNODES}
            "./src/main.cpp"
            "./src/app.hpp"
            "./src/dockspace.cpp"
            "./src/dockspace.hpp"
            "./src/schematic.cpp"
            "./src/schematic.hpp"
            "./src/schematic_block.cpp"
            "./src/schematic_block.hpp"
            "./src/status_bar.cpp"
            "./src/status_bar.hpp"
            "./src/debug_console.cpp"
            "./src/debug_console.hpp"
            "./src/schematic_editor.cpp"
            "./src/schematic_editor.hpp"
            "./src/window_object.hpp"
            "./src/block_editor.cpp"
            "./src/block_editor.hpp"
            "./src/librarian.hpp"
            )

add_subdirectory("libs/glfw")
include_directories("libs/glfw/include")
include_directories("libs/imgui")
include_directories("libs/imgui/backends")
include_directories("libs/boost")
include_directories("libs/imgui-node-editor")
include_directories("libs/imnodes")


find_package(OpenGL REQUIRED)
target_link_libraries(PLCEditio OpenGL::GL)

target_link_libraries(PLCEditio glfw ${GLFW_LIBRARIES})
target_link_libraries(PLCEditio glfw ${OPENGL_glu_LIBRARY} ${GLFW_LIBRARIES})


target_link_libraries(PLCEditio "${CMAKE_SOURCE_DIR}/libs/boost/stage/lib/libboost_filesystem-vc143-mt-gd-x64-1_80.lib")
target_link_libraries(PLCEditio "${CMAKE_SOURCE_DIR}/libs/boost/stage/lib/libboost_json-vc143-mt-gd-x64-1_80.lib")
target_link_libraries(PLCEditio "${CMAKE_SOURCE_DIR}/libs/boost/stage/lib/libboost_container-vc143-mt-gd-x64-1_80.lib")



target_compile_definitions(PLCEditio PRIVATE PLC_EDITIO_WINDOW_NAME="PLC Editio" )



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

