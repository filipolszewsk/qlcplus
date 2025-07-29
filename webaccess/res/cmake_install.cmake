# Install script for directory: /Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res

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
   "/Users/filipolszewski/QLC+.app/Contents/Resources/Web/common.css;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/keypad.html;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/networkconfig.js;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/simpledesk.css;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/simpledesk.js;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/virtualconsole.css;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/virtualconsole.js;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/configuration.js;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/websocket.js;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/favicon.ico;/Users/filipolszewski/QLC+.app/Contents/Resources/Web/favicon-192x192.png")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/filipolszewski/QLC+.app/Contents/Resources/Web" TYPE FILE FILES
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/common.css"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/keypad.html"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/networkconfig.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/simpledesk.css"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/simpledesk.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/virtualconsole.css"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/virtualconsole.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/configuration.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/websocket.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/favicon.ico"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/webaccess/res/favicon-192x192.png"
    )
endif()

