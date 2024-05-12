#!/bin/sh

echo "conan install . --output-folder=$1 -g CMakeToolchain -pr:b conanprofile.$2.txt -pr:h conanprofile.$2.txt --build=missing"
conan install . --output-folder=$1 -g CMakeToolchain -pr:b conanprofile.$2.txt -pr:h conanprofile.$2.txt --build=missing
