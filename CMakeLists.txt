cmake_minimum_required(VERSION 3.20)

### before-project config ###

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

#############################


### project setup ###

project(change-me
    VERSION 1.0.0
    HOMEPAGE_URL "https://github.com/JungerBoyo/minimal-cpp-template-project"
)

######################


### sub dirs ###

if (DEFINED SANITIZE_ADDRESS)
    if (DEFINED SANITIZE_THREAD)
        message(FATAL_ERROR "Can't combine thread and address sanitizer")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    endif()
endif()
if (DEFINED SANITIZE_MEMORY)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(WARNING "Memory sanitizer not offcially supported for GCC")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
    endif()
endif()
if (DEFINED SANITIZE_THREAD)
    if (DEFINED SANITIZE_LEAK)
        message(FATAL_ERROR "Can't combine thread and leak sanitizer")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    endif()
endif()
if (DEFINED SANITIZE_UNDEFINED_BEHAVIOUR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()
if (DEFINED SANITIZE_LEAK)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
endif()


add_subdirectory(src)

################

### add conan install as dependency ###
### package project ###
install(TARGETS change-me RUNTIME DESTINATION "./")
# install(TARGETS some-lib DESTINATION "./lib/")
# install(FILES 
#     ${CMAKE_CURRENT_SOURCE_DIR}/.../some_include.h
#     DESTINATION "./include/")
# 
# install(DIRECTORY ${PROJECT_SOURCE_DIR}/.../some_dir DESTINATION "./some_dir/")

include(CPack)
########################
