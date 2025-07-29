# Install script for directory: /Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates

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
   "/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Always_Full.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Always_Half.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Always_Off.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Exponential_Deep.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Exponential_Medium.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Exponential_Shallow.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Exponential_Simple.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Invert.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Linear.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Logarithmic_Deep.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Logarithmic_Medium.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Logarithmic_Shallow.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Preheat_5_Percent.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/S-curve.qxmt;/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates/Threshold.qxmt")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/filipolszewski/QLC+.app/Contents/Resources/ModifiersTemplates" TYPE FILE FILES
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Always_Full.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Always_Half.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Always_Off.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Exponential_Deep.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Exponential_Medium.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Exponential_Shallow.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Exponential_Simple.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Invert.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Linear.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Logarithmic_Deep.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Logarithmic_Medium.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Logarithmic_Shallow.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Preheat_5_Percent.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/S-curve.qxmt"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/modifierstemplates/Threshold.qxmt"
    )
endif()

