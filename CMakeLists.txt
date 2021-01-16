cmake_minimum_required(VERSION 3.1.0)

project(resetmsmice VERSION 1.2.0)
include(CPack)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if (USEGUI AND "${USEGUI}" MATCHES "OFF")
  message(STATUS "Not building with GUI components")
  set (USEQT "OFF")
  set (USEGTK "OFF")
else()
  set (USEGUI "ON")
  message(STATUS "Building with GUI components, set USEGUI=OFF to build without")
endif()

if (APPLE)
  find_package(PkgConfig)
  find_package(Iconv REQUIRED)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O2 -Wall -Wextra")
  if ("${USEGUI}" MATCHES "ON")
    set(USEQT "ON")
    set(USEGTK "OFF")
  endif()
elseif (MSVC)
  find_package(LibUSB REQUIRED libusb-1.0)
  find_package(Iconv REQUIRED)
  if ("${USEGUI}" MATCHES "ON")
    set(USEQT "ON")
    set(USEGTK "OFF")
  endif()
else()
  find_package(PkgConfig)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0) 
  set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O2 -Wall -Wextra")
  if ("${USEGUI}" MATCHES "ON")
    if (USEQT AND "${USEQT}" MATCHES "ON")
      message(STATUS "Building with Qt5")
      find_package(Qt5Core REQUIRED)
      find_package(Qt5Widgets REQUIRED)
    else()
      set (USEGTK "ON")
      message(STATUS "Building with Gtk3")
      message(STATUS "set USEQT=ON to build with Qt5")
    endif()
  endif()
endif()

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set (MAIN_SRC
  src/resetmsmice.c
  src/main.c 
  src/parse.c
)

if (APPLE)
  list(APPEND MAIN_SRC src/pthread-misc.c)
  set(resetmsmice_ICON ${CMAKE_CURRENT_SOURCE_DIR}/data/mouse.icns)
  set(GUI_TYPE MACOSX_BUNDLE)
  set(MACOSX_BUNDLE TRUE)
  if ("${USEGUI}" MATCHES "OFF")
    set(MACOSX_BUNDLE_BUNDLE_NAME resetmsmice)
    set(MACOSX_BUNDLE_STARTUP_COMMAND resetmsmice)
  else()
    set(MACOSX_BUNDLE_BUNDLE_NAME resetmsmice-gui)
    set(MACOSX_BUNDLE_STARTUP_COMMAND resetmsmice-gui)
  endif()
  set(MACOSX_BUNDLE_ICON_FILE mouse.icns)
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "resetmsmice version ${CMAKE_PROJECT_VERSION}")
  set(MACOSX_GUI_INDENTIFIER resetmsmice)
  set_source_files_properties(${resetmsmice_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set(RESETMSMICE_BUNDLE_PREFIXA ${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_BUNDLE_NAME}.app/Contents)
  set(RESETMSMICE_BUNDLE_PREFIXB ${PROJECT_BINARY_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}.app/Contents)

  set(RESETMSMICE_BUNDLE_LOCA ${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_BUNDLE_NAME}.app)
  set(RESETMSMICE_BUNDLE_LOCB ${PROJECT_BINARY_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}.app)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/macos/resetmsmice-enable-boot PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()


set (MAIN_HEADERS
  include/resetmsmice.h
  include/hid-userland.h
  include/pthread-misc.h
  include/basictypes.h
)

#---------------------------------------------------------------------------
# Default Build: command line binary resetmsmice
#---------------------------------------------------------------------------
if (APPLE AND "${USEGUI}" MATCHES "OFF")
  add_executable(resetmsmice MACOSX_BUNDLE ${resetmsmice_ICON} ${MAIN_SRC} ${MAIN_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/macos/resetmsmice-enable-boot)
else()
  add_executable(resetmsmice ${MAIN_SRC} ${MAIN_HEADERS})
endif()
target_link_libraries(resetmsmice Threads::Threads ${Iconv_LIBRARIES} ${LIBUSB_LIBRARIES})

#---------------------------------------------------------------------------
# Build For Qt 
#---------------------------------------------------------------------------
if (USEQT AND "${USEQT}" MATCHES "ON")
  find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  set (GUI_SRC
    src/qt5/main.cc
    src/qt5/resetWindow.cc
    src/qt5/execSave.cc
    src/qt5/eventLoopProcess.cc
    src/qt5/terminalView.cc
  )

  set (GUI_HEADERS
    include/qt5/execSave.h
    include/qt5/resetWindow.h
    include/qt5/eventLoopProcess.h
    include/qt5/terminalView.h
  )

  include_directories( ${LIBUSB_INCLUDE_DIRS} ${THREADS_INCLUDE_DIRS} ${Iconv_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/qt5)

  if (APPLE)
    include(InstallRequiredSystemLibraries)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployQt5.cmake)
    add_executable(resetmsmice-gui MACOSX_BUNDLE ${resetmsmice_ICON} ${GUI_SRC} ${GUI_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/macos/resetmsmice-enable-boot)
    add_dependencies(resetmsmice-gui resetmsmice)
    target_link_libraries(resetmsmice-gui Qt5::Widgets "-framework CoreFoundation")
  else()
    add_executable(resetmsmice-gui ${GUI_SRC} ${GUI_HEADERS})
    target_link_libraries(resetmsmice-gui Qt5::Widgets)
  endif()


#---------------------------------------------------------
# Build For GTK
#---------------------------------------------------------
elseif (USEGTK AND "${USEGTK}" MATCHES "ON")

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

  set (GUI_SRC
    src/gtk3/execSave.cc
    src/gtk3/resetWindow.cc
  )

  set (GUI_HEADERS
    include/gtk3/execSave.h
    include/gtk3/resetWindow.h
  )

  include_directories( ${GTK3_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIRS} ${THREADS_INCLUDE_DIRS} ${Iconv_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/gtk3)
  link_directories(${GTK3_LIBRARY_DIRS})

  add_definitions(${GTK3_CFLAGS_OTHER})
  add_executable(resetmsmice-gui ${GUI_SRC} ${GUI_HEADERS})

  target_link_libraries(resetmsmice-gui ${GTK3_LIBRARIES})
endif()


#------------------------------------------------------------------------
# Install for Windows
#------------------------------------------------------------------------
if (WIN32)
  install(DIRECTORY "resetmsmice" DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(TARGETS resetmsmice 
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/resetmsmice)
  if ("${USEGUI}" MATCHES "ON")
    install(TARGETS resetmsmice-gui 
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/resetmsmice)
  endif()
#------------------------------------------------------------------------
# Default ElseIf / Install for Unix/Linux other than Windows or 
# Mac OS for basic binaries
#------------------------------------------------------------------------
elseif (NOT APPLE)
  install(TARGETS resetmsmice 
          RUNTIME DESTINATION bin)
  if ("${USEGUI}" MATCHES "ON")
    install(TARGETS resetmsmice-gui
            RUNTIME DESTINATION bin)
  endif()
endif()


#------------------------------------------------------------------------
# Install for Mac OS
#------------------------------------------------------------------------
if (APPLE)
  set(CPACK_PACKAGE_FILE_NAME "${MACOSX_BUNDLE_BUNDLE_NAME}")
#  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/macos/resetmsmice-enable-boot PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
  if ("${USEGUI}" MATCHES "ON")
    install(TARGETS resetmsmice-gui
            BUNDLE DESTINATION .)
    install(TARGETS resetmsmice 
            DESTINATION ${RESETMSMICE_BUNDLE_PREFIXB}/Resources)
    set(QT_PLUGINS)
    foreach(PLUGIN ${Qt5Widgets_PLUGINS})
      get_target_property(_loc ${PLUGIN} LOCATION)
      list(APPEND QT_PLUGINS ${_loc})
    endforeach()
    install_qt5_executable("resetmsmice-gui.app" "${QT_PLUGINS}" "" "${LIB_DIRS}" "")
  else()
    install(TARGETS resetmsmice 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
            BUNDLE DESTINATION .)
  endif()

#  install(CODE "
#          include(BundleUtilities)
          # fixup_bundle tries to copy system libraries without this.
#          function(gp_resolved_file_type_override file type)
#            if(file MATCHES \"^(/usr/lib)\" AND NOT file MATCHES \"cocoa\")
#              set(type \"system\" PARENT_SCOPE)
#            endif()
#          endfunction()
#         set(BU_CHMOD_BUNDLE_ITEMS ON)
#         fixup_bundle(\"${RESETMSMICE_BUNDLE_LOCA}\" \"${QT_PLUGINS}\" \"${LIB_DIRS}\")
#         fixup_bundle(\"${RESETMSMICE_BUNDLE_LOCB}\" \"${QT_PLUGINS}\" \"${LIB_DIRS}\")
#         ")

#------------------------------------------------------------------------
# Post Install for Windows
#------------------------------------------------------------------------
elseif (WIN32)
  install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinstall-windows.bat)")
#------------------------------------------------------------------------
# More Install Scripts/Data for Linux
#------------------------------------------------------------------------
else()
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/60-resetmsmice.rules PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ DESTINATION /etc/udev/rules.d)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/resetmsmice.desktop PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ DESTINATION /usr/share/applications)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/linux/resetmsmice-enable-boot PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION sbin)
  set(ENV{"RESETMSMICESBIN"} "${CMAKE_INSTALL_PREFIX}/sbin")
  install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinstall)")
endif()

