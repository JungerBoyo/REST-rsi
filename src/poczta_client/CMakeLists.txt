find_package(CLI11 REQUIRED)
find_package(PNG REQUIRED)
find_package(base64 REQUIRED)

set(SUBPROJECT_NAME "${PROJECT_NAME}")

add_executable(${SUBPROJECT_NAME} client.cpp)
target_link_libraries(${SUBPROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        Pistache::Pistache
        nlohmann_json::nlohmann_json
        CLI11::CLI11
        PNG::PNG
        aklomp::base64
)

#add_dependencies(change-me some-dependency)

#install(TARGETS ${SUBPROJECT_NAME} RUNTIME DESTINATION "./${SUBPROJECT_NAME}")