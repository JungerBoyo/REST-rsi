find_package(RapidJSON REQUIRED)

set(SUBPROJECT_NAME "${PROJECT_NAME}-lab11")

add_executable(${SUBPROJECT_NAME} main.cpp)

add_executable(${SUBPROJECT_NAME}-client client.cpp)

target_link_libraries(${SUBPROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        Pistache::Pistache
        nlohmann_json::nlohmann_json
        rapidjson
)
target_link_libraries(${SUBPROJECT_NAME}-client
    PRIVATE
        spdlog::spdlog
        Pistache::Pistache
        nlohmann_json::nlohmann_json
)

#add_dependencies(change-me some-dependency)

#install(TARGETS ${SUBPROJECT_NAME} RUNTIME DESTINATION "./${SUBPROJECT_NAME}")