:: Build script for appveyor, https://www.appveyor.com
:: Builds one version linked against wxWidgets 3.1 and one against 3.2


:: Common setup
::
@echo off
setlocal enabledelayedexpansion

set "SCRIPTDIR=%~dp0"
set "GIT_HOME=C:\Program Files\Git"
if "%CONFIGURATION%" == "" set "CONFIGURATION=RelWithDebInfo"

:: Loop to build two versions
set "wx_vers=wx32"
:loop
  echo Building !wx_vers!
  
  call %SCRIPTDIR%..\buildwin\win_deps.bat !wx_vers!
  call %SCRIPTDIR%..\cache\wx-config.bat
  echo USING wxWidgets_LIB_DIR: !wxWidgets_LIB_DIR!
  echo USING wxWidgets_ROOT_DIR: !wxWidgets_ROOT_DIR!
  echo USING OCPN_TARGET_TUPLE: !TARGET_TUPLE!
  
  nmake /?  >nul 2>&1
  if errorlevel 1 (
    set "VS_HOME=C:\Program Files\Microsoft Visual Studio\2022"
    call "%VS_HOME%\Community\VC\Auxiliary\Build\vcvars32.bat"
  )
  
  if exist build (rmdir /s /q build)
  mkdir build && cd build
  
  cmake -A Win32 -G "Visual Studio 17 2022" ^
      -DCMAKE_GENERATOR_PLATFORM=Win32 ^
      -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
      -DwxWidgets_LIB_DIR=!wxWidgets_LIB_DIR! ^
      -DwxWidgets_ROOT_DIR=!wxWidgets_ROOT_DIR! ^
      -DOCPN_TARGET_TUPLE=!TARGET_TUPLE! ^
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
if "!wx_vers!" == "wx32" (set "wx_vers=wx31" && goto loop)
