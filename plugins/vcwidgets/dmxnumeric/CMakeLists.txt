cmake_minimum_required(VERSION 3.16)
project(dmxnumeric_vcwidget)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# -----------------------------------------------------------------------
# Locate QLC+ libraries from its build directory
# -----------------------------------------------------------------------
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

# -----------------------------------------------------------------------
# Plugin target
# -----------------------------------------------------------------------
add_library(dmxnumeric_vcwidget MODULE
    dmxnumericplugin.cpp dmxnumericplugin.h
    dmxnumericwidget.cpp dmxnumericwidget.h
    dmxnumericconfigdialog.cpp dmxnumericconfigdialog.h
)

target_compile_features(dmxnumeric_vcwidget PRIVATE cxx_std_14)

# Use platform-specific shared library suffix expected by QLC+'s plugin loader:
# .dylib on macOS, .so on Linux, .dll on Windows.
if(APPLE)
    set_target_properties(dmxnumeric_vcwidget PROPERTIES SUFFIX ".dylib")
endif()

target_link_libraries(dmxnumeric_vcwidget
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${QLCPLUSUI_LIB}
        ${QLCPLUSENGINE_LIB}
)

target_include_directories(dmxnumeric_vcwidget
    PRIVATE
        "${QLCPLUS_SRC_DIR}/ui/src"
        "${QLCPLUS_SRC_DIR}/ui/src/virtualconsole"
        "${QLCPLUS_SRC_DIR}/engine/src"
)

install(TARGETS dmxnumeric_vcwidget
    LIBRARY DESTINATION vcwidgets
    RUNTIME DESTINATION vcwidgets
)
