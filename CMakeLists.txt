cmake_minimum_required(VERSION 3.23.0)

project(clang-format-all)

add_executable(${PROJECT_NAME} "src/main.cxx")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    EXPORT_COMPILE_COMMANDS TRUE
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/install")

enable_testing()
add_test(NAME HelloHelp COMMAND $<TARGET_FILE:${PROJECT_NAME}> --help WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
