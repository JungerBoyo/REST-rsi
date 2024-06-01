set(SUBPROJECT_NAME "${PROJECT_NAME}-lab10")

add_executable(${SUBPROJECT_NAME} main.cpp)

target_link_libraries(${SUBPROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        Pistache::Pistache
        nlohmann_json::nlohmann_json
)

#add_dependencies(change-me some-dependency)

install(TARGETS ${SUBPROJECT_NAME} RUNTIME DESTINATION "./${SUBPROJECT_NAME}")