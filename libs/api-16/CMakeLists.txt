cmake_minimum_required(VERSION 3.1)

if (TARGET ocpn::api)
  return ()
endif ()

add_library(OCPN_API INTERFACE)
add_library(ocpn::api ALIAS OCPN_API)
target_include_directories(OCPN_API INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
if (WIN32 AND MSVC)
  target_link_libraries(
    OCPN_API INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/msvc/opencpn.lib")
  # Assuming unicode build, required for mingw headers
  target_compile_definitions(OCPN_API INTERFACE -DUNICODE)
elseif(WIN32 AND MINGW) 
  target_link_libraries(
    OCPN_API INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/mingw/libopencpn.dll.a")
endif ()

