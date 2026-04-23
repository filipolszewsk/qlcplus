cmake_minimum_required(VERSION 3.16)
project(icons VERSION 1.0 LANGUAGES C CXX)

include(../../variables.cmake)

# install macOS files for a correct bundle
install(FILES qt.conf DESTINATION ${INSTALLROOT}/${DATADIR})
install(FILES ../../resources/icons/qlcplus.icns DESTINATION ${INSTALLROOT}/${DATADIR})

if(qmlui)
    install(FILES Info.plist.qmlui DESTINATION ${INSTALLROOT} RENAME Info.plist)
else()
    install(FILES Info.plist DESTINATION ${INSTALLROOT})
endif()
# NOTE: we use `sed -i.bak` (not `-i -e`) because BSD sed on macOS parses
# the `-e` after `-i` as the backup suffix and silently writes Info.plist-e,
# which later breaks codesign with "code object is not signed at all /
# In subcomponent: .../Info.plist-e". `.bak` works on both BSD and GNU sed.
install(CODE "execute_process(COMMAND sed -i.bak -e \"s/__QLC_VERSION__/${APPVERSION}/g\" ${INSTALLROOT}/Info.plist)")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E rm -f ${INSTALLROOT}/Info.plist.bak)")

# Qt frameworks and plugins are bundled by macdeployqt during the DMG creation step.
# Do NOT install Qt here - macdeployqt handles dependency resolution and rpath rewriting.

# install support libraries (non-Qt, not bundled by macdeployqt)
install(FILES ${FFTW3_LIBDIR}/libfftw3.3.dylib DESTINATION ${INSTALLROOT}/${LIBSDIR})
install(FILES ${LIBFTDI1_libftdi1_LIBDIR}/libftdi1.2.dylib DESTINATION ${INSTALLROOT}/${LIBSDIR})
install(FILES ${LIBFTDI1_libftdi1_LIBDIR}/libftdi1.2.5.0.dylib DESTINATION ${INSTALLROOT}/${LIBSDIR})
install(FILES ${LIBUSB1_LIBDIR}/libusb-1.0.0.dylib DESTINATION ${INSTALLROOT}/${LIBSDIR})
install(FILES ${SNDFILE_LIBDIR}/libsndfile.1.dylib DESTINATION ${INSTALLROOT}/${LIBSDIR})
install(FILES ${SNDFILE_LIBDIR}/libsndfile.1.0.37.dylib DESTINATION ${INSTALLROOT}/${LIBSDIR})
