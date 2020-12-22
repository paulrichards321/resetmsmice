cmake_minimum_required(VERSION 3.1.0)

project(resetmsmice)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if (APPLE)
  find_package(PkgConfig)
  find_package(Iconv REQUIRED)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
  set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -Wall -Wextra")
  set(USEQT "ON")
  set(USEGTK "OFF")
elseif (MSVC)
  find_package(LibUSB REQUIRED libusb-1.0)
  find_package(Iconv REQUIRED)
  set (USEQT "ON")
else()
  find_package(PkgConfig)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0) 
  set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -Wall -Wextra")
  if (USEQT AND "${USEQT}" MATCHES "ON")
    message(STATUS "Building with Qt5")
  else()
    set (USEGTK "ON")
    message(STATUS "Building with Gtk3")
    message(STATUS "set USEQT=ON to build with Qt5")
  endif()
endif()

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (USEQT AND "${USEQT}" MATCHES "ON")
  find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

  set ( MAIN_SRC
    src/main.c 
    src/parse.c
  )

  set ( MAIN_HEADERS
    include/hid-userland.h
    include/pthread-misc.h
  )

  set ( GUI_SRC
    src/qt5/main.cc
    src/qt5/resetWindow.cc
    src/qt5/execSave.cc
    src/qt5/eventLoopProcess.cc
    src/qt5/terminalView.cc
  )

  set ( GUI_HEADERS
    include/qt5/execSave.h
    include/qt5/resetWindow.h
    include/qt5/eventLoopProcess.h
    include/qt5/terminalView.h
  )

  set ( UIS
    include/qt5/resetWindow.ui
  )

  include_directories( ${LIBUSB_INCLUDE_DIRS} ${THREADS_INCLUDE_DIRS} ${Iconv_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/qt5)

  if (APPLE)
    list(APPEND MAIN_SRC src/pthread-misc.c)
  endif()

  add_executable(resetmsmice ${MAIN_SRC} ${MAIN_HEADERS})

  add_executable(resetmsmice-gui ${GUI_SRC} ${UIS} ${GUI_HEADERS})

  target_link_libraries(resetmsmice Threads::Threads ${Iconv_LIBRARIES} ${LIBUSB_LIBRARIES})

  target_link_libraries(resetmsmice-gui Qt5::Widgets)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

  set ( MAIN_SRC
    src/main.c 
    src/parse.c
  )

  set ( MAIN_HEADERS
    include/hid-userland.h
    include/pthread-misc.h
  )

  set ( GUI_SRC
    src/gtk3/execSave.cc
    src/gtk3/resetWindow.cc
  )

  set ( GUI_HEADERS
    include/gtk3/execSave.h
    include/gtk3/resetWindow.h
  )

  include_directories( ${GTK3_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIRS} ${THREADS_INCLUDE_DIRS} ${Iconv_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/gtk3)
  link_directories(${GTK3_LIBRARY_DIRS})

  if (APPLE)
    list(APPEND MAIN_SRC src/pthread-misc.c)
  endif()

  add_executable(resetmsmice ${MAIN_SRC} ${MAIN_HEADERS})

  add_definitions(${GTK3_CFLAGS_OTHER})
  add_executable(resetmsmice-gui ${GUI_SRC} ${GUI_HEADERS})

  target_link_libraries(resetmsmice Threads::Threads ${Iconv_LIBRARIES} ${LIBUSB_LIBRARIES})

  target_link_libraries(resetmsmice-gui ${GTK3_LIBRARIES})
endif()

if (WIN32)
  install(DIRECTORY "resetmsmice" DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(TARGETS resetmsmice resetmsmice-gui
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/resetmsmice)
else()
  install(TARGETS resetmsmice resetmsmice-gui 
          RUNTIME DESTINATION sbin)
endif()

if (APPLE)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/scripts/resetmsmice-enable-boot-macosx PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION sbin RENAME resetmsmice-enable-boot)
  install(CODE "execute_process(COMMAND ${CMAKE_INSTALL_PREFIX}/sbin/resetmsmice-enable-boot)")
elseif (WIN32)
  install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinstall-windows.bat)")
else()
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/60-resetmsmice.rules PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ DESTINATION /etc/udev/rules.d)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/resetmsmice.desktop PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ DESTINATION /usr/share/applications)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/scripts/resetmsmice-enable-boot-linux PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION sbin RENAME resetmsmice-enable-boot)
  set(ENV{"RESETMSMICESBIN"} "${CMAKE_INSTALL_PREFIX}/sbin")
  install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinstall-linux)")
endif()

