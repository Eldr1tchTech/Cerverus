#!/bin/bash

set echo on

echo "Copying assets..."

mkdir -p bin/
rm -rf bin/assets
cp -r assets bin/

echo "Assets copied successfully."