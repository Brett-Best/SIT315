#!/bin/bash

declare -a Schemes=("Matrix Multiplication - Sequential" "Matrix Multiplication - Parallel" "Matrix Multiplication - OpenMP")

mkdir Archives;

for scheme in "${Schemes[@]}"; do
  xcodebuild -quiet -workspace "Parallel Matrix Multiplication.xcworkspace" -scheme "$scheme" -configuration Release -archivePath "$PWD/Archive" clean build archive;
  mv "Archive.xcarchive/Products/usr/local/bin/$scheme" "Archives/$Scheme";
  rm -rf Archive.xcarchive
done
