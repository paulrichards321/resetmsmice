cmake_minimum_required(VERSION 3.1.0)

project(resetmsmice)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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
  set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -Wall -Wextra")
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
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -Wall -Wextra")
  if ("${USEGUI}" MATCHES "ON")
    if (USEQT AND "${USEQT}" MATCHES "ON")
      message(STATUS "Building with Qt5")
    else()
      set (USEGTK "ON")
      message(STATUS "Building with Gtk3")
      message(STATUS "set USEQT=ON to build with Qt5")
    endif()
  endif()
endif()

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set ( MAIN_SRC
  src/resetmsmice.c
  src/main.c 
  src/parse.c
)

if (APPLE)
  list(APPEND MAIN_SRC src/pthread-misc.c)
endif()

set ( MAIN_HEADERS
  include/resetmsmice.h
  include/hid-userland.h
  include/pthread-misc.h
  include/basictypes.h
)

add_executable(resetmsmice ${MAIN_SRC} ${MAIN_HEADERS})
target_link_libraries(resetmsmice Threads::Threads ${Iconv_LIBRARIES} ${LIBUSB_LIBRARIES})

if (USEQT AND "${USEQT}" MATCHES "ON")
  find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

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

  include_directories( ${LIBUSB_INCLUDE_DIRS} ${THREADS_INCLUDE_DIRS} ${Iconv_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/qt5)

  add_executable(resetmsmice-gui ${GUI_SRC} ${UIS} ${GUI_HEADERS})

  target_link_libraries(resetmsmice-gui Qt5::Widgets)
elseif (USEGTK AND "${USEGTK}" MATCHES "ON")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

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

  add_definitions(${GTK3_CFLAGS_OTHER})
  add_executable(resetmsmice-gui ${GUI_SRC} ${GUI_HEADERS})

  target_link_libraries(resetmsmice-gui ${GTK3_LIBRARIES})
endif()

if (WIN32)
  install(DIRECTORY "resetmsmice" DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(TARGETS resetmsmice 
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/resetmsmice)
  if ("${USEGUI}" MATCHES "ON")
    install(TARGETS resetmsmice-gui 
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/resetmsmice)
  endif()
else()
  install(TARGETS resetmsmice 
          RUNTIME DESTINATION sbin)
  if ("${USEGUI}" MATCHES "ON")
    install(TARGETS resetmsmice-gui
            RUNTIME DESTINATION bin)
  endif()
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

