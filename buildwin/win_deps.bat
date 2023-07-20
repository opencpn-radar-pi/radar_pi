:: Install build dependencies. Requires a working choco installation,
:: see https://docs.chocolatey.org/en-us/choco/setup.
::
:: Usage:
::      win_deps.bat [wx32]
::
:: Arguments:
::     wx32:
::          If present install dependencies for building against
::          wxWidgets 3.2. If non-existing or anything but wx32
::          build using 3.1
:: Output:
::     cache\wx-config.bat:
::          Script which set wxWidgets_LIB_DIR and wxWidgets_ROOT_DIR
::
:: Initial run will do choco installs requiring administrative
:: privileges.
::
:: NOTE: at the very end, this script runs refreshenv. This clears the
:: process's PATH and replaces it with a fresh copy from the
:: registry. This means that any "set PATH=..." is lost for caller.

:: Install the pathman tool: https://github.com/therootcompany/pathman
:: Fix PATH so it can be used in this script
::

@echo off

setlocal enabledelayedexpansion

if not exist "%HomeDrive%%HomePath%\.local\bin\pathman.exe" (
    pushd "%HomeDrive%%HomePath%\.local\bin"
    powershell /? >nul 2>&1
    if errorlevel 1 set "PATH=%PATH%;C:\Windows\System32\WindowsPowerShell\v1.0"
    curl.exe -sA "windows/10 x86"  -o webi-pwsh-install.ps1 ^
        https://webi.ms/packages/_webi/webi-pwsh.ps1
    powershell.exe -ExecutionPolicy Bypass -File webi-pwsh-install.ps1
    webi.bat pathman
    popd
)
pathman list > nul 2>&1
if errorlevel 1 set "PATH=%PATH%;%HomeDrive%\%HomePath%\.local\bin"
pathman add %HomeDrive%%HomePath%\.local\bin >nul

git --version > nul 2>&1
if errorlevel 1 (
   set "GIT_HOME=C:\Program Files\Git"
   if not exist "!GIT_HOME!" choco install -y git
   pathman add !GIT_HOME!\cmd > nul
)

:: Install choco cmake and add it's persistent user path element
::
cmake --version > nul 2>&1
if errorlevel 1 (
  set "CMAKE_HOME=C:\Program Files\CMake"
  choco install -y cmake
  pathman add "!CMAKE_HOME!\bin" > nul
  echo Adding !CMAKE_HOME!\bin to path
)

:: Install choco poedit and add it's persistent user path element
::
set "POEDIT_HOME=C:\Program Files (x86)\Poedit\Gettexttools"
if not exist "%POEDIT_HOME%" choco install -y poedit
pathman add "%POEDIT_HOME%\bin" > nul

:: Update required python stuff
::
python --version > nul 2>&1 && python -m ensurepip > nul 2>&1
if errorlevel 1 choco install -y python
python --version
python -m ensurepip
python -m pip install --upgrade pip
python -m pip install -q setuptools wheel
python -m pip install -q cloudsmith-cli
python -m pip install -q cryptography

::
:: Install pre-compiled wxWidgets and other DLL; add required paths.
::
set SCRIPTDIR=%~dp0
if "%~1"=="wx32" (
  set "WXWIN=%SCRIPTDIR%..\cache\wxWidgets-3.2.2.1"
  set "wxWidgets_ROOT_DIR=!WXWIN!"
  set "wxWidgets_LIB_DIR=!WXWIN!\lib\vc14x_dll"
) else (
  set "WXWIN=%SCRIPTDIR%..\cache\wxWidgets-3.2.2.1"
  set "wxWidgets_ROOT_DIR=!WXWIN!"
  set "wxWidgets_LIB_DIR=!WXWIN!\lib\vc_dll"
)

if not exist %SCRIPTDIR%\..\cache ( mkdir %SCRIPTDIR%\..\cache )
set "CONFIG_FILE=%SCRIPTDIR%\..\cache\wx-config.bat"
echo set "wxWidgets_ROOT_DIR=%wxWidgets_ROOT_DIR%" > %CONFIG_FILE%
echo set "wxWidgets_LIB_DIR=%wxWidgets_LIB_DIR%" >> %CONFIG_FILE%

if not exist "%WXWIN%" (
  wget --version > nul 2>&1 || choco install -y wget
  if  "%~1"=="wx32" (
      echo Downloading 3.2.2.1
      if not exist  %SCRIPTDIR%..\cache\wxWidgets-3.2.2.1 (
          mkdir %SCRIPTDIR%..\cache\wxWidgets-3.2.2.1
      )
      set "GITHUB_DL=https://github.com/wxWidgets/wxWidgets/releases/download"
      wget -nv !GITHUB_DL!/v3.2.2.1/wxMSW-3.2.2_vc14x_Dev.7z
      7z x -o%SCRIPTDIR%..\cache\wxWidgets-3.2.2.1 wxMSW-3.2.2_vc14x_Dev.7z
      wget -nv !GITHUB_DL!/v3.2.2.1/wxWidgets-3.2.2.1-headers.7z
      7z x -o%SCRIPTDIR%..\cache\wxWidgets-3.2.2.1 wxWidgets-3.2.2.1-headers.7z
  ) else (
      echo Downloading 3.1.2
      wget -O wxWidgets-3.1.2.7z -nv ^
        https://download.opencpn.org/s/E2p4nLDzeqx4SdX/download
      7z i > nul 2>&1 || choco install -y 7zip
      7z x wxWidgets-3.1.2.7z -o%WXWIN%
  )
)

refreshenv
