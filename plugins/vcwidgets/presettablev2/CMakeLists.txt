cmake_minimum_required(VERSION 3.16)
project(presettablev2_vcwidget)

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

add_library(presettablev2_vcwidget MODULE
    presettablev2plugin.cpp presettablev2plugin.h
    presettablev2widget.cpp presettablev2widget.h
    presettablev2configdialog.cpp presettablev2configdialog.h
    presettablev2columndialog.cpp presettablev2columndialog.h
    presettablev2effectengine.cpp presettablev2effectengine.h
    ptdimmerwaveengine.cpp ptdimmerwaveengine.h
    ptparammatrixengine.cpp ptparammatrixengine.h
    ptspatialfixtureplan.cpp ptspatialfixtureplan.h
    ptspatialfixturegridwidget.cpp ptspatialfixturegridwidget.h
    presettablev2controliface.h
    presettablev2multibuttoniface.h
    presettablev2transitionprovideriface.h
    presettablev2vclookup.cpp presettablev2vclookup.h
)

target_compile_features(presettablev2_vcwidget PRIVATE cxx_std_17)

if(APPLE)
    set_target_properties(presettablev2_vcwidget PROPERTIES SUFFIX ".dylib")
endif()
if(WIN32)
    set_target_properties(presettablev2_vcwidget PROPERTIES PREFIX "")
endif()

target_link_libraries(presettablev2_vcwidget
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${QLCPLUSUI_LIB}
        ${QLCPLUSENGINE_LIB}
)

target_include_directories(presettablev2_vcwidget
    PRIVATE
        "${QLCPLUS_SRC_DIR}/ui/src"
        "${QLCPLUS_SRC_DIR}/ui/src/virtualconsole"
        "${QLCPLUS_SRC_DIR}/engine/src"
        "${QLCPLUS_BUILD_DIR}/ui/src/qlcplusui_autogen/include"
)

install(TARGETS presettablev2_vcwidget
    LIBRARY DESTINATION vcwidgets
    RUNTIME DESTINATION vcwidgets
)
