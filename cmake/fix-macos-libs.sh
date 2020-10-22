#!/bin/bash

readonly RUNTIME_PATH="/Applications/OpenCPN.app/Contents/Frameworks"

plugin=$(find app/files -name '*.dylib')

for lib in $(otool -L "$plugin" | grep wx | awk '{print $1}'); do
    libdir=${lib%/*}
    if [ "$libdir" = "$lib" ]; then
        continue
    elif [ "$libdir" != "$RUNTIME_PATH" ]; then
        runtime_lib=$(echo $lib | sed "s|$libdir|$RUNTIME_PATH|")
        install_name_tool -change "$lib" "$runtime_lib" "$plugin"
    fi
done
echo "fix-macos-libs.sh: Revised library paths:"
otool -L "$plugin" | grep wx
