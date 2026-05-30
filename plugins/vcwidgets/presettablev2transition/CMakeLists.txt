cmake_minimum_required(VERSION 3.16)
project(presettablev2transition_vcwidget)

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

add_library(presettablev2transition_vcwidget MODULE
    presettablev2transitionplugin.cpp presettablev2transitionplugin.h
    presettablev2transitionwidget.cpp presettablev2transitionwidget.h
    presettablev2transitionconfigdialog.cpp presettablev2transitionconfigdialog.h
    presettablev2transitioncolumndialog.cpp presettablev2transitioncolumndialog.h
    ../presettablev2/presettablev2effectengine.cpp ../presettablev2/presettablev2effectengine.h
    ../presettablev2/ptdimmerwaveengine.cpp ../presettablev2/ptdimmerwaveengine.h
    ../presettablev2/ptdimmerwavecurvewidget.cpp ../presettablev2/ptdimmerwavecurvewidget.h
    ../presettablev2/ptparammatrixengine.cpp ../presettablev2/ptparammatrixengine.h
    ../presettablev2/presettablev2controliface.h
    ../presettablev2/presettablev2transitionprovideriface.h
    ../presettablev2/presettablev2vclookup.cpp ../presettablev2/presettablev2vclookup.h
    ../presettablev2/ptefxinputids.h
)

target_compile_features(presettablev2transition_vcwidget PRIVATE cxx_std_17)

if(APPLE)
    set_target_properties(presettablev2transition_vcwidget PROPERTIES SUFFIX ".dylib")
endif()
if(WIN32)
    set_target_properties(presettablev2transition_vcwidget PROPERTIES PREFIX "")
endif()

target_link_libraries(presettablev2transition_vcwidget
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${QLCPLUSUI_LIB}
        ${QLCPLUSENGINE_LIB}
)

target_include_directories(presettablev2transition_vcwidget
    PRIVATE
        "${QLCPLUS_SRC_DIR}/ui/src"
        "${QLCPLUS_SRC_DIR}/ui/src/virtualconsole"
        "${QLCPLUS_SRC_DIR}/engine/src"
        "${QLCPLUS_BUILD_DIR}/ui/src/qlcplusui_autogen/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../presettablev2"
)

install(TARGETS presettablev2transition_vcwidget
    LIBRARY DESTINATION vcwidgets
    RUNTIME DESTINATION vcwidgets
)
