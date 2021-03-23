rem
rem Install build dependencies. Requires a working choco installation,
rem see https://docs.chocolatey.org/en-us/choco/setup. 
rem
rem Initial run will do choco installs requiring administrative 
rem privileges.
rem


SET PATH=C:\Program Files\CMake\bin;C:\Python38;C:\Python38\Scripts;%PATH%
python --version > nul 2>&1 || choco install -y python
python --version
python -m ensurepip
python -m pip install --upgrade pip
python -m pip install -q setuptools wheel
python -m pip install -q cloudsmith-cli
python -m pip install -q cryptography

set WXWIN=C:\wxWidgets-3.1.2
set wxWidgets_ROOT_DIR=%WXWIN%
set wxWidgets_LIB_DIR=%WXWIN%\lib\vc_dll
SET PATH=%PATH%;%WXWIN%;%wxWidgets_LIB_DIR%
if not exist %WXWIN% (
  wget --version > nul 2>&1 || choco install -y wget
  wget https://download.opencpn.org/s/E2p4nLDzeqx4SdX/download -O wxWidgets-3.1.2.7z
  7z i > nul 2>&1 || choco install -y 7z
  7z x wxWidgets-3.1.2.7z -o%WXWIN%
)

if not exist "C:\Program Files (x86)\Poedit" (
  choco install -y  poedit
)
SET PATH=%PATH%;C:\Program Files (x86)\Poedit\Gettexttools\bin
SET PATH=%PATH%;C:\Program Files (x86)\Poedit
