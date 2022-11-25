:: Build script for appveyor, https://www.appveyor.com
:: Builds one version linked against wxWidgets 3.1 and one against 3.2


:: Common setup
::
@echo off
setlocal enabledelayedexpansion

set "SCRIPTDIR=%~dp0"
set "GIT_HOME=C:\Program Files\Git"

::   wxWidgets 3.2 version
::
echo Building using wxWidgets 3.2

call %SCRIPTDIR%..\buildwin\win_deps.bat wx32
call %SCRIPTDIR%..\cache\wx-config.bat
echo USING wxWidgets_LIB_DIR: !wxWidgets_LIB_DIR!
echo USING wxWidgets_ROOT_DIR: !wxWidgets_ROOT_DIR!

nmake /?  >nul 2>&1
if errorlevel 1 (
  set "VS_HOME=C:\Program Files\Microsoft Visual Studio\2022"
  call "%VS_HOME%\Community\VC\Auxiliary\Build\vcvars32.bat"
)

if exist build-wx32 (rmdir /s /q build-wx32)
mkdir build-wx32 && cd build-wx32
set "WXWIN=!wxWidgets_ROOT_DIR!
cmake -A Win32 -G "Visual Studio 17 2022" ^
    -DCMAKE_GENERATOR_PLATFORM=Win32 ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    -DwxWidgets_LIB_DIR=!wxWidgets_LIB_DIR! ^
    -DwxWidgets_ROOT_DIR=!wxWidgets_ROOT_DIR! ^
    ..
cmake --build . --target tarball --config %CONFIGURATION%

if exist "%GIT_HOME%\bin\bash.exe" (
  echo Library runtime linkage:
  "%GIT_HOME%\bin\bash" -c "ldd app/*/plugins/*dll"
)

echo Uploading artifact
call upload.bat

echo Pushing updates to catalog
python %SCRIPTDIR%..\ci\git-push
cd ..

::   wxWidgets 3.1 version
::
echo Building using wxWidgets 3.1
call %SCRIPTDIR%..\buildwin\win_deps.bat
call %SCRIPTDIR%..\cache\wx-config.bat

nmake /? >nul 2>&1
if errorlevel 1 (
  set "VS_HOME=C:\Program Files\Microsoft Visual Studio\2022"
  call "%VS_HOME%\Community\VC\Auxiliary\Build\vcvars32.bat"
)

echo USING wxWidgets_ROOT_DIR: %wxWidgets_ROOT_DIR%
echo USING wxWidgets_LIB_DIR: %wxWidgets_LIB_DIR%

if exist build (rmdir /s /q build)
mkdir build && cd build
cmake -A Win32 -G "Visual Studio 17 2022" ^
    -DCMAKE_GENERATOR_PLATFORM=Win32 ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    -DwxWidgets_LIB_DIR=%wxWidgets_LIB_DIR% ^
    -DwxWidgets_ROOT_DIR=%wxWidgets_ROOT_DIR% ^
    ..
cmake --build . --target tarball --config %CONFIGURATION%

if exist "%GIT_HOME%\bin\bash.exe" (
  echo Library runtime linkage: 
  "%GIT_HOME%\bin\bash" -c "ldd app/*/plugins/*dll"
)

echo Uploading artifact
call %SCRIPTDIR%..\build\upload.bat

echo Pushing updates to catalog
python %SCRIPTDIR%..\ci\git-push
cd ..
