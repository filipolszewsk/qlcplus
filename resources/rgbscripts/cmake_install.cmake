# Install script for directory: /Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts

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
   "/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/alternate.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/balls.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/blinder.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/circles.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/circular.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/evenodd.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/fill.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/fillfromcenter.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/fillunfill.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/fillunfillfromcenter.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/fillunfillsquaresfromcenter.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/fireworks.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/flyingobjects.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/gradient.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/lines.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/marquee.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/noise.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/onebyone.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/opposite.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/plasma.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randomcolumn.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randomfillcolumn.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randomfillrow.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randomfillsingle.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randompixelperrow.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randompixelperrowmulticolor.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randomrow.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/randomsingle.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/sinewave.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/snowbubbles.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/squares.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/squaresfromcenter.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/starfield.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/stripes.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/stripesfromcenter.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/strobe.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/verticalfall.js;/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts/waves.js")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/filipolszewski/QLC+.app/Contents/Resources/RGBScripts" TYPE FILE FILES
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/alternate.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/balls.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/blinder.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/circles.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/circular.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/evenodd.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/fill.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/fillfromcenter.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/fillunfill.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/fillunfillfromcenter.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/fillunfillsquaresfromcenter.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/fireworks.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/flyingobjects.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/gradient.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/lines.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/marquee.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/noise.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/onebyone.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/opposite.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/plasma.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randomcolumn.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randomfillcolumn.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randomfillrow.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randomfillsingle.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randompixelperrow.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randompixelperrowmulticolor.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randomrow.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/randomsingle.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/sinewave.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/snowbubbles.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/squares.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/squaresfromcenter.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/starfield.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/stripes.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/stripesfromcenter.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/strobe.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/verticalfall.js"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/rgbscripts/waves.js"
    )
endif()

