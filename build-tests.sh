#!/bin/bash
# Build script for rebuilding tests
set echo on

echo "Building everything..."


# pushd tests
# source build.sh
# popd
make -f Makefile.tests.linux.mak all

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Killing previous tasks..."

pkill -f './project' || killall project 2>/dev/null