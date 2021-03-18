@echo on

setlocal enableextensions

if "%1" == "" (
  echo Usage: build.cmd [ide^|cli]
  echo.
  echo Build script for radar_pi
  echo -------------------------
  echo.
  echo This script is intended for use by @canboat, @hakansv and @douwefokkema only.
  echo We use it to set preconditions on our build machines to run in IDE.
  echo.
  echo Any others: use at own risk.
  echo.
  echo Feel free to adapt this to your own situation.
  echo.
  goto :EOF
)

set "BUILD_TYPE=%1"
set "BUILDDIR=rel-%COMPUTERNAME%"
set "STATE=Unofficial"
set "TARGET=pkg"
set "CMAKE_OPTIONS="
set "MAKE_OPTIONS="

set "CMAKE_BUILD_PARALLEL_LEVEL=2"

rem On Windows, ide means Visual Studio
if "%BUILD_TYPE%" == "ide" (
  set "BUILDDIR=%BUILDDIR%-vs"
)

if "%COMPUTERNAME%" == "PVW10X64" ( 
  rem Adapt for Kees VM on kees-mbp
  set "WXWIN=Y:\src\wx312_win32"
  call :setup_kees
) else if "%COMPUTERNAME%" == "W7" (
  rem Adapt for Kees VM on kees-mbp
  set "WXWIN=E:\src\wx312_win32"
  call :setup_kees
) else if "%COMPUTERNAME%" == "HakansV" (
  rem This is an example 
)

if exist "%BUILDDIR%" (
  rmdir /s/q "%BUILDDIR%"
)
mkdir "%BUILDDIR%"
cd "%BUILDDIR%"

cmake -T v141_xp -G "Visual Studio 15 2017" --config Debug -DBUILD_TYPE="%BUILD_TYPE%" %CMAKE_OPTIONS% ..
if errorlevel 1 goto :EOF
if "%BUILD_TYPE%" == "ide" (
  start radar_pi.sln
) else (
  dir
  cmake --build .
)
goto :EOF

:setup_kees
  set "PATH=C:\Windows\system32;C:\windows"
  set "PATH=%PATH%;c:\Program Files (x86)\nasm"
  set "PATH=%PATH%;C:\Program Files (x86)\poedit\gettexttools\bin"
  set "PATH=%PATH%;C:\Program Files\cmake\bin"
  set "PATH=%PATH%;C:\Python36;c:\Python36\Scripts"
  call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
  set "wxWidgets_ROOT_DIR=%WXWIN%"
  set "wxWidgets_LIB_DIR=%WXWIN%\lib\vc_dll"
  set "PATH=%PATH%;%WXWIN%;%wxWidgets_LIB_DIR%"
  goto :EOF

