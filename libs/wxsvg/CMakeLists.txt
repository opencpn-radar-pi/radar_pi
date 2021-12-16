#
#  The wxsvg library is part of main OpenCPN. Some functions
#  are exported in the API, which also requires headers. This
#  library  provides headers and the necessary compilation definitions.
#
#  In particular, it defines the ocpnUSE_SVG preprocessor symbol.

cmake_minimum_required(VERSION 3.1)

if (TARGET ocpn::wxsvg)
  return()
endif ()

add_library(_WXSVG INTERFACE)
add_library(ocpn::wxsvg ALIAS _WXSVG)
target_include_directories(_WXSVG INTERFACE include)
target_compile_definitions(_WXSVG INTERFACE ocpnUSE_SVG)
