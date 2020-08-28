# ~~~
# Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
# Copyright:   2014
# License:     GPLv3+
# ~~~

if (NOT APPLE)
  target_link_libraries(${PACKAGE_NAME} ${wxWidgets_LIBRARIES} ${EXTRA_LIBS})
endif (NOT APPLE)

if (WIN32)
  set(PARENT "opencpn")

  if (MSVC)
    # TARGET_LINK_LIBRARIES(${PACKAGE_NAME} gdiplus.lib glu32.lib)
    target_link_libraries(${PACKAGE_NAME} ${OPENGL_LIBRARIES})
    set(OPENCPN_IMPORT_LIB "${PROJECT_SOURCE_DIR}/api-16/msvc/opencpn.lib")
  endif (MSVC)

  if (MINGW)
    # assuming wxwidgets is compiled with unicode, this is needed for mingw
    # headers
    add_definitions(" -DUNICODE")
    target_link_libraries(${PACKAGE_NAME} ${OPENGL_LIBRARIES})
    set(OPENCPN_IMPORT_LIB
        "${PROJECT_SOURCE_DIR}/api-16/mingw/libopencpn.dll.a"
    )
    set(CMAKE_SHARED_LINKER_FLAGS "-L../buildwin")
  endif (MINGW)

  target_link_libraries(${PACKAGE_NAME} ${OPENCPN_IMPORT_LIB})
endif (WIN32)

if (UNIX)
  if (PROFILING)
    find_library(
      GCOV_LIBRARY
      NAMES gcov
      PATHS /usr/lib/gcc/i686-pc-linux-gnu/4.7
    )

    set(EXTRA_LIBS ${EXTRA_LIBS} ${GCOV_LIBRARY})
  endif (PROFILING)
endif (UNIX)

if (APPLE)
  install(
    TARGETS ${PACKAGE_NAME}
    RUNTIME
    LIBRARY DESTINATION OpenCPN.app/Contents/SharedSupport/plugins
  )
  find_package(ZLIB REQUIRED)
  target_link_libraries(${PACKAGE_NAME} ${ZLIB_LIBRARIES})
  install(
    TARGETS ${PACKAGE_NAME}
    RUNTIME
    LIBRARY DESTINATION OpenCPN.app/Contents/PlugIns
  )

  if (EXISTS ${PROJECT_SOURCE_DIR}/data)
    install(
      DIRECTORY data
      DESTINATION OpenCPN.app/Contents/SharedSupport/plugins/${PACKAGE_NAME}
    )
  endif ()

endif (APPLE)

if (UNIX AND NOT APPLE)
  find_package(BZip2 REQUIRED)
  include_directories(${BZIP2_INCLUDE_DIR})
  find_package(ZLIB REQUIRED)
  include_directories(${ZLIB_INCLUDE_DIR})
  target_link_libraries(${PACKAGE_NAME} ${BZIP2_LIBRARIES} ${ZLIB_LIBRARY})
endif (UNIX AND NOT APPLE)

set(PARENT opencpn)

set(PREFIX_DATA share)
set(PREFIX_LIB lib)

if (WIN32)
  message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
  set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/../OpenCPN)
  if (CMAKE_CROSSCOMPILING)
    install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
    set(INSTALL_DIRECTORY "plugins/${PACKAGE_NAME}")
  else (CMAKE_CROSSCOMPILING)
    install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
    set(INSTALL_DIRECTORY "plugins\\\\${PACKAGE_NAME}")
  endif (CMAKE_CROSSCOMPILING)

  if (EXISTS ${PROJECT_SOURCE_DIR}/data)
    install(DIRECTORY data DESTINATION "${INSTALL_DIRECTORY}")
  endif (EXISTS ${PROJECT_SOURCE_DIR}/data)
endif (WIN32)

if (UNIX AND NOT APPLE)
  set(PREFIX_PARENTDATA ${PREFIX_DATA}/${PARENT})
  if (NOT DEFINED PREFIX_PLUGINS)
    set(PREFIX_PLUGINS ${PREFIX_LIB}/${PARENT})
  endif (NOT DEFINED PREFIX_PLUGINS)
  install(
    TARGETS ${PACKAGE_NAME}
    RUNTIME
    LIBRARY DESTINATION ${PREFIX_PLUGINS}
  )

  if (EXISTS ${PROJECT_SOURCE_DIR}/data)
    install(DIRECTORY data
            DESTINATION ${PREFIX_PARENTDATA}/plugins/${PACKAGE_NAME}
    )
  endif ()
endif (UNIX AND NOT APPLE)
