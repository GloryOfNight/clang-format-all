cmake_minimum_required(VERSION 3.21.0)

project(clang-format-all
        VERSION 1.4
        DESCRIPTION "Tool to help reformat directories of C/Cpp files"
        HOMEPAGE_URL "https://github.com/GloryOfNight/clang-format-all"
        LANGUAGES CXX C)

include(cmake/ProjectOptions.cmake)
include(cmake/Sanitizers.cmake)

add_executable(${PROJECT_NAME} "src/main.cxx")

enable_sanitizers(${PROJECT_NAME} 
                    ${ENABLE_SANITIZER_ADDRESS} 
                    ${ENABLE_SANITIZER_LEAK} 
                    ${ENABLE_SANITIZER_UNDEFINED} 
                    ${ENABLE_SANITIZER_THREAD} 
                    ${ENABLE_SANITIZER_MEMORY})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/install")

enable_testing()
add_test(NAME HelloHelp COMMAND $<TARGET_FILE:${PROJECT_NAME}> --help WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
