conan install . --output-folder=build-rel -g CMakeToolchain -pr:b conanprofile.linux.txt -pr:h conanprofile.linux.txt --build=missing && \
cmake -S . -B build-rel -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
cmake --build build-rel


