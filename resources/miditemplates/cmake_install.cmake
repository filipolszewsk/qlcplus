# Install script for directory: /Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/miditemplates

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/filipolszewski/QLC+.app/Contents/Resources/MidiTemplates/APC20.qxm;/Users/filipolszewski/QLC+.app/Contents/Resources/MidiTemplates/APC40.qxm;/Users/filipolszewski/QLC+.app/Contents/Resources/MidiTemplates/APC40MK2.qxm;/Users/filipolszewski/QLC+.app/Contents/Resources/MidiTemplates/APCmini.qxm;/Users/filipolszewski/QLC+.app/Contents/Resources/MidiTemplates/APCminiMK2.qxm;/Users/filipolszewski/QLC+.app/Contents/Resources/MidiTemplates/Novation-LaunchPadMiniMK3.qxm")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/filipolszewski/QLC+.app/Contents/Resources/MidiTemplates" TYPE FILE FILES
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/miditemplates/APC20.qxm"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/miditemplates/APC40.qxm"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/miditemplates/APC40MK2.qxm"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/miditemplates/APCmini.qxm"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/miditemplates/APCminiMK2.qxm"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/miditemplates/Novation-LaunchPadMiniMK3.qxm"
    )
endif()

