@echo off
echo Build script for BR24radar_pi
echo -----------------------------
echo.
echo This script is intended for use by @canboat only, otherwise he forgets which VM to build
echo on such that it works on as many systems as possible."
echo.
echo Feel free to adapt this to your own situation.

set BUILDDIR=build-win32
set CMAKE_TARGET=
set PACKAGE=br24radar_pi*-win32.exe
set VARS="C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
set TARGET=

rem Official build machines
if "%COMPUTERNAME%" == "MERRIMAC1" (
    echo Running on "Merrimac" vbox with wxWidgets 2.8.12 with VS 2010.
    set VARS="C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
    set OPENCPNLIB=buildwin\opencpn_v400_wx28.lib
    set CMAKE_TARGET=
    set TARGET=--config release --target package
)

if "%TARGET%" == "" (
  echo Building unofficial release, without packages.
  set TARGET=--build .
  set STATE=unofficial
  set PACKAGE=
) else (
  for /f %%l in (release-state) do set STATE=%%l
  if "%STATE" == "" (
    echo Please set release state of package
    exit 1
  )
  echo -------------------------- BUILD %STATE% -----------------------
)

call %VARS%

if exist %BUILDDIR% rmdir /s /q %BUILDDIR%
mkdir %BUILDDIR%
copy %OPENCPNLIB% %BUILDDIR%\opencpn.lib
cd %BUILDDIR%
cmake .. %CMAKE_TARGET%

echo -------------------------- MAKE %TARGET% -----------------------
cmake --build . %TARGET%

if exist ..\..\OpenCPN-Navico-Radar-Plugin.github.io\%STATE% if not "%PACKAGE%" == "" (
  echo -------------------------- COPY FILES TO %STATE% -----------------------
  copy %PACKAGE% ..\..\OpenCPN-Navico-Radar-Plugin.github.io\%STATE%
)

cd ..
  
