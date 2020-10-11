#!/bin/sh

readonly RUNTIME_PATH="/Applications/OpenCPN.app/Contents/Frameworks"

dylib=$(find app/files -name '*.dylib')

for lib in $(otool -L $dylib | grep wx | awk '{print $1}'); do
    libdir=${lib%/*}
    if [ "$libdir" = "$lib" ]; then
        continue
    elif [ "$libdir" != "$RUNTIME_PATH" ]; then
        newlib=$(echo $lib | sed "s|$libdir|$RUNTIME_PATH|")
        install_name_tool -change $lib $newlib $dylib
    fi
done
echo "Revised library paths:"
otool -L $dylib | grep wx

