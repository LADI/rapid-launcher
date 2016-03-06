#!/bin/sh

cd "$(dirname $0)"
export LD_LIBRARY_PATH="../Appman/build/" 
export GI_TYPELIB_PATH="../Appman/build/"
./rapidlauncher
