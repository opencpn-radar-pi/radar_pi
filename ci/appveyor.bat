:: Build script for appveyor, https://www.appveyor.com
:: Builds one version linked against wxWidgets 3.2

@echo on
setlocal enabledelayedexpansion

set "SCRIPTDIR=%~dp0"
set "GIT_HOME=C:\Program Files\Git"

:: %CONFIGURATION% comes from appveyor.yml, set a default if invoked elsewise.
if "%CONFIGURATION%" == "" set "CONFIGURATION=RelWithDebInfo"

call %SCRIPTDIR%..\buildwin\win_deps.bat wx32
call %SCRIPTDIR%..\cache\wx-config.bat
echo USING wxWidgets_LIB_DIR: !wxWidgets_LIB_DIR!
echo USING wxWidgets_ROOT_DIR: !wxWidgets_ROOT_DIR!

if not defined VCINSTALLDIR (
  for /f "tokens=* USEBACKQ" %%p in (
    `"%programfiles(x86)%\Microsoft Visual Studio\Installer\vswhere" ^
    -latest -property installationPath`
  ) do call "%%p\Common7\Tools\vsDevCmd.bat"
)

if exist build (rmdir /s /q build)
mkdir build && cd build

:: This id done in win_deps.bat, but for some reason stopped
:: working since 2024-02-28, so work it around here
set "PATH=%PATH%;C:\Program Files (x86)\Poedit\Gettexttools\bin"

cmake -A Win32 -G "Visual Studio 17 2022" ^
    -DCMAKE_GENERATOR_PLATFORM=Win32 ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    -DwxWidgets_LIB_DIR=!wxWidgets_LIB_DIR! ^
    -DwxWidgets_ROOT_DIR=!wxWidgets_ROOT_DIR! ^
    -DOCPN_TARGET_TUPLE=msvc-wx32;10;x86_64 ^
    ..
cmake --build . --target tarball --config %CONFIGURATION%

:: Display dependencies debug info
echo import glob; import subprocess > ldd.py
echo lib = glob.glob("app/*/plugins/*.dll")[0] >> ldd.py
echo subprocess.call(['dumpbin', '/dependents', lib], shell=True) >> ldd.py
python ldd.py

echo Uploading artifact
call upload.bat

echo Pushing updates to catalog
python %SCRIPTDIR%..\ci\git-push
cd ..
