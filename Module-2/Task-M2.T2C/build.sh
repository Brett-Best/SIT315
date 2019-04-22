#!/bin/bash

declare -a Schemes=("Quicksort - Sequential" "Quicksort - Parallel")

mkdir Archives;

for scheme in "${Schemes[@]}"; do
  xcodebuild -quiet -workspace "Parallel Quicksort.xcworkspace" -scheme "$scheme" -configuration Release -archivePath "$PWD/Archive" clean build archive;
  mv "Archive.xcarchive/Products/usr/local/bin/$scheme" "Archives/$Scheme";
  rm -rf Archive.xcarchive
done
