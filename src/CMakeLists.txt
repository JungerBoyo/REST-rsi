find_package(spdlog REQUIRED)

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    # placeholder
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    # placeholder
endif()

add_executable(change-me main.cpp)

target_link_libraries(change-me PRIVATE spdlog::spdlog)

#add_dependencies(change-me some-dependency)
