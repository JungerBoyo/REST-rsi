find_package(CLI11 REQUIRED)
set(SUBPROJECT_NAME "${PROJECT_NAME}-lab13")

add_executable(${SUBPROJECT_NAME}-server server.cpp)

add_executable(${SUBPROJECT_NAME}-client client.cpp)

target_link_libraries(${SUBPROJECT_NAME}-server
    PRIVATE
        spdlog::spdlog
        Pistache::Pistache
        nlohmann_json::nlohmann_json
        CLI11::CLI11
)
target_link_libraries(${SUBPROJECT_NAME}-client
    PRIVATE
        spdlog::spdlog
        Pistache::Pistache
        nlohmann_json::nlohmann_json
        CLI11::CLI11
)

#add_dependencies(change-me some-dependency)

#install(TARGETS ${SUBPROJECT_NAME} RUNTIME DESTINATION "./${SUBPROJECT_NAME}")