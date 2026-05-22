cmake_minimum_required(VERSION 3.16)
project(hello_vcwidget)

# This plugin must be compiled with the SAME Qt version and compiler as QLC+.
# Check the QLC+ release notes for the exact Qt version used per platform.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# -----------------------------------------------------------------------
# Plugin target
# MODULE = shared library not linked into other targets (Qt plugin convention)
# -----------------------------------------------------------------------
add_library(hello_vcwidget MODULE
    helloplugin.cpp helloplugin.h
    hellowidget.cpp hellowidget.h
)

target_compile_features(hello_vcwidget PRIVATE cxx_std_14)

target_link_libraries(hello_vcwidget
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        # Link against the QLC+ UI library so VCWidget / Doc are available.
        # Adjust the path to point at your local QLC+ build directory.
        # Example (macOS build in build/):
        #   ${QLCPLUS_BUILD_DIR}/ui/src/libqlcplusui.dylib
        qlcplusui
        qlcplusengine
)

target_include_directories(hello_vcwidget
    PRIVATE
        # Adjust these paths to your QLC+ source tree.
        ${QLCPLUS_SRC_DIR}/ui/src/virtualconsole
        ${QLCPLUS_SRC_DIR}/engine/src
)

# -----------------------------------------------------------------------
# Install
# On macOS the default install prefix is set to the .app bundle.
# Adjust as needed for your platform.
# -----------------------------------------------------------------------
install(TARGETS hello_vcwidget
    LIBRARY DESTINATION vcwidgets
    RUNTIME DESTINATION vcwidgets
)
