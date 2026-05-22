cmake_minimum_required(VERSION 3.16)
project(infinityencoder_vcwidget)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

if(NOT DEFINED QLCPLUS_SRC_DIR)
    message(FATAL_ERROR "Pass -DQLCPLUS_SRC_DIR=/path/to/qlcplus/source")
endif()
if(NOT DEFINED QLCPLUS_BUILD_DIR)
    message(FATAL_ERROR "Pass -DQLCPLUS_BUILD_DIR=/path/to/qlcplus/build")
endif()

find_library(QLCPLUSUI_LIB
    NAMES qlcplusui
    HINTS "${QLCPLUS_BUILD_DIR}/ui/src"
    REQUIRED
)
find_library(QLCPLUSENGINE_LIB
    NAMES qlcplusengine
    HINTS "${QLCPLUS_BUILD_DIR}/engine/src"
    REQUIRED
)

message(STATUS "qlcplusui:     ${QLCPLUSUI_LIB}")
message(STATUS "qlcplusengine: ${QLCPLUSENGINE_LIB}")

add_library(infinityencoder_vcwidget MODULE
    infinityencoderplugin.cpp infinityencoderplugin.h
    infinityencoderwidget.cpp infinityencoderwidget.h
    infinityencoderconfigdialog.cpp infinityencoderconfigdialog.h
)

target_compile_features(infinityencoder_vcwidget PRIVATE cxx_std_17)

if(APPLE)
    set_target_properties(infinityencoder_vcwidget PROPERTIES SUFFIX ".dylib")
endif()
if(WIN32)
    set_target_properties(infinityencoder_vcwidget PROPERTIES PREFIX "")
endif()

target_link_libraries(infinityencoder_vcwidget
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${QLCPLUSUI_LIB}
        ${QLCPLUSENGINE_LIB}
)

target_include_directories(infinityencoder_vcwidget
    PRIVATE
        "${QLCPLUS_SRC_DIR}/ui/src"
        "${QLCPLUS_SRC_DIR}/ui/src/virtualconsole"
        "${QLCPLUS_SRC_DIR}/engine/src"
)

install(TARGETS infinityencoder_vcwidget
    LIBRARY DESTINATION vcwidgets
    RUNTIME DESTINATION vcwidgets
)
