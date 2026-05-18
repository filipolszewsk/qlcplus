cmake_minimum_required(VERSION 3.16)
project(dmxnumeric_vcwidget)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_library(dmxnumeric_vcwidget MODULE
    dmxnumericplugin.cpp dmxnumericplugin.h
    dmxnumericwidget.cpp dmxnumericwidget.h
)

target_compile_features(dmxnumeric_vcwidget PRIVATE cxx_std_14)

target_link_libraries(dmxnumeric_vcwidget
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        qlcplusui
        qlcplusengine
)

target_include_directories(dmxnumeric_vcwidget
    PRIVATE
        ${QLCPLUS_SRC_DIR}/ui/src/virtualconsole
        ${QLCPLUS_SRC_DIR}/engine/src
)

install(TARGETS dmxnumeric_vcwidget
    LIBRARY DESTINATION vcwidgets
    RUNTIME DESTINATION vcwidgets
)
