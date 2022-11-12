cmake_minimum_required(VERSION 3.1.0)

if (TARGET ocpn::zlib)
    return ()
endif ()

add_library(_ZLIB_IF INTERFACE)
add_library(ocpn::zlib ALIAS _ZLIB_IF)

if (WIN32)
  add_library(_ZLIB_MSVC STATIC IMPORTED)
  set_property(
    TARGET _ZLIB_MSVC
    PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/win/zlib1.lib
  )
  target_include_directories(_ZLIB_IF INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/include/zlib
  )
  target_link_libraries(_ZLIB_IF INTERFACE _ZLIB_MSVC)
else ()
  find_package(ZLIB REQUIRED)
  target_link_libraries(_ZLIB_IF INTERFACE ZLIB::ZLIB)
endif ()
