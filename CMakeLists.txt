cmake_minimum_required(VERSION 3.8.0)

project(clang-format-all)

add_executable(${PROJECT_NAME} "src/main.cxx")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    EXPORT_COMPILE_COMMANDS TRUE
)

# to support #include <execution> on UNIX must link tbb and pthread library
if(UNIX)
find_library(TBB_LIB NAMES tbb REQUIRED)
target_link_libraries(${PROJECT_NAME} "tbb" "pthread")
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/install")