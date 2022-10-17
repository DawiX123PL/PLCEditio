cmake_minimum_required(VERSION 3.8)



project("PLCEditio")


add_executable(PLCEditio "src/main.cpp")



set_target_properties(PLCEditio
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/out"
)



# set language standard to c++11
if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(PLCEditio PROPERTY CXX_STANDARD 20)
    set_property(PLCEditio PROPERTY CXX_STANDARD 20)
endif()

