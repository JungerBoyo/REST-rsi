# minimal-cpp-template-project
[![ci](https://github.com/JungerBoyo/minimal-cpp-template-project/actions/workflows/ci.yml/badge.svg)](https://github.com/JungerBoyo/minimal-cpp-template-project/actions/workflows/ci.yml)

Cpp cmake template project which is meant to be simple namely, easily understandable and modifiable. Works on windows and linux. For linux default is `GCC` compiler for windows default is `Clang` compiler. Project also implements github CI pipeline compiling and cpacking + publishing project for windows and linux. Compile happens on every commit, cpacking and publishing on a release.

## Package manager

Template project uses conan as its package manager. Conan profiles are stored in `conanprofile.linux.txt` and `conanprofile.windows.txt`, conan file is `conanfile.py`. To install conan packages and configure it in `build` directory for linux do eg:
```
  ./install_packages.sh build linux
                          ^     ^------ linux/windows
                          * -- generator
```

## CMake configuration
To configure (after installing conan packages, if there is any conan configuration) run eg:
```
  ./configure build Ninja Debug ...other
                ^    ^      ^------ build type
                |    * -- generator
                *--- build directory
```
You can specifies additional variables that should be appended to cmake conf. Specific to project variables are ones enabling sanitizers:
  *  (-D)SANITIZE_ADDRESS(=ON)
  *  (-D)SANITIZE_THREAD(=ON)
  *  (-D)SANITIZE_UNDEFINED_BEHAVIOUR(=ON)
  *  (-D)SANITIZE_LEAK(=ON)
  *  (-D)SANITIZE_MEMORY(=ON) (works only for clang)

## Building
To build run command: 
```
  ./build.sh build
               ^------ build directory
```
