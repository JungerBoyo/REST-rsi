find_package(spdlog REQUIRED)
find_package(Pistache REQUIRED)
# find_package(pugixml REQUIRED)
find_package(nlohmann_json REQUIRED)

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    # placeholder
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    # placeholder
endif()

add_subdirectory(lab9)
add_subdirectory(lab10)
add_subdirectory(lab11)
add_subdirectory(lab12)
add_subdirectory(lab13)
