
BR24radar_pi - Navico Broadband Radar Plugin for OpenCPN
========================================================

Downloads
---------

The latest binary version for Microsoft Windows can be downloaded at http://opencpn-navico-radar-plugin.github.io .
We will add Mac and Linux versions over time.

Compiling
---------
* This plugin now builds out of the OpenCPN source tree
```
git clone https://github.com/canboat/BR24radar_pi.git
```

###Build:
```
mkdir BR24radar_pi/build
cd BR24radar_pi/build
cmake ..
cmake --build .
```
Windows note: You must place opencpn.lib into your build directory to be able to link the plugin DLL. You can get this file from your local OpenCPN build, or alternatively download from http://sourceforge.net/projects/opencpnplugins/files/opencpn_lib/

Debugging:
If you check out the plugin source into the plugins subdirectory of your OpenCPN source tree, you can build it as part of it (exactly as with the versions prior to 1.0)

###Creating a package
Linux
```
make package
```

Windows
```
cmake --build . --config release --target package
```

###Build on Mac OS X:
Tools: Can be installed either manually or from Homebrew (http://brew.sh)
```
#brew install git #If I remember well, it is already available on the system
brew install cmake
brew install gettext
ln -s /usr/local/Cellar/gettext/0.19.2/bin/msgmerge /usr/local/bin/msgmerge
ln -s /usr/local/Cellar/gettext/0.19.2/bin/msgfmt /usr/local/bin/msgfmt
```

To target older OS X versions than the one you are running, you need the respective SDKs installed. The easiest way to achieve that is using https://github.co

####Building wxWidgets
(do not use wxmac from Homebrew, it is not compatible with OpenCPN)
Get wxWidgets 3.0.x source from http://wxwidgets.org
Configure, build and install
```
cd wxWidgets-3.0.2
./configure --enable-unicode --with-osx-cocoa --with-macosx-sdk=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7
make
sudo make install
```

####Building the plugin
Before running cmake, you must set the deployment target to OS X 10.7 to be compatible with the libraries used by core OpenCPN
```
export MACOSX_DEPLOYMENT_TARGET=10.7
```

####Packaging on OS X
Get and install the Packages application from http://s.sudre.free.fr/Software/Packages/about.html
```
make create-pkg
```

License
-------
The plugin code is licensed under the terms of the GPL v2.

Acknowledgements
----------------
The authors wish to thank Dave Register for providing OpenCPN and the Garmin radar plugin, from which this plugin was derived.
