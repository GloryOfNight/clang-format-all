cmake_minimum_required(VERSION 3.0.0)
project(clang-format-all)

add_executable(${PROJECT_NAME} "src/main.cxx")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    EXPORT_COMPILE_COMMANDS TRUE
)

install(TARGETS ${PROJECT_NAME})