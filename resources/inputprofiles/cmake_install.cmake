# Install script for directory: /Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles

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
   "/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/ADJ-MIDICON-2.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Akai-APC20.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Akai-APC40-mkII.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Akai-APC40.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Akai-APCMini-mk2.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Akai-APCMini.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Behringer-BCF2000.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Behringer-BCF2000inMackieControlMode.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Behringer-BCR2000.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Behringer-LC2412.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Behringer-X-TouchExtender.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Elation-MIDIcon.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Enttec-PlaybackWing.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Enttec-ShortcutWing.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Generic-MIDI.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/KORG-nanoKONTROL.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/KORG-nanoKONTROL2.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/KORG-nanoKONTROLStudio.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/KORG-nanoPAD.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/KORG-nanoPAD2.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Lemur-iPadStudioCombo.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Logitech-WingManAttack2.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/M-Wave-SMCMixer.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Mixxx-MIDI.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Novation-LaunchControl.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Novation-LaunchControlXL.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Novation-LaunchPadMiniMK3.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Novation-Launchpad.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Novation-LaunchpadMK2.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Novation-LaunchpadPro.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/PMJ-9-Faders-Controller.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/PMJ-Circus.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/PMJ-MidiKey.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/ShowTec-ShowMaster24.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/TouchOSC-Automat5.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/TouchOSC-Mix16.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Worlde-Easypad.12.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Worlde-OrcaPAD16.qxi;/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles/Zoom-R16.qxi")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/filipolszewski/QLC+.app/Contents/Resources/InputProfiles" TYPE FILE FILES
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/ADJ-MIDICON-2.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Akai-APC20.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Akai-APC40-mkII.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Akai-APC40.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Akai-APCMini-mk2.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Akai-APCMini.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Behringer-BCF2000.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Behringer-BCF2000inMackieControlMode.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Behringer-BCR2000.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Behringer-LC2412.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Behringer-X-TouchExtender.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Elation-MIDIcon.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Enttec-PlaybackWing.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Enttec-ShortcutWing.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Generic-MIDI.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/KORG-nanoKONTROL.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/KORG-nanoKONTROL2.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/KORG-nanoKONTROLStudio.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/KORG-nanoPAD.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/KORG-nanoPAD2.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Lemur-iPadStudioCombo.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Logitech-WingManAttack2.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/M-Wave-SMCMixer.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Mixxx-MIDI.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Novation-LaunchControl.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Novation-LaunchControlXL.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Novation-LaunchPadMiniMK3.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Novation-Launchpad.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Novation-LaunchpadMK2.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Novation-LaunchpadPro.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/PMJ-9-Faders-Controller.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/PMJ-Circus.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/PMJ-MidiKey.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/ShowTec-ShowMaster24.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/TouchOSC-Automat5.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/TouchOSC-Mix16.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Worlde-Easypad.12.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Worlde-OrcaPAD16.qxi"
    "/Users/filipolszewski/Documents/qlc projekty/qlcplus/resources/inputprofiles/Zoom-R16.qxi"
    )
endif()

