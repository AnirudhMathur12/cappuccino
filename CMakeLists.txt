cmake_minimum_required(VERSION 3.12)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Extract version from tag (assumes tags like v1.1.0)
    if(GIT_TAG MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(VERSION_FROM_GIT "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
        message(STATUS "Version from git tag: ${VERSION_FROM_GIT}")
    endif()
endif()

if(VERSION_FROM_GIT)
    set(PROJECT_VERSION_OVERRIDE ${VERSION_FROM_GIT})
else()
    set(PROJECT_VERSION_OVERRIDE "1.1.0")
endif()

project(cappuccino 
    VERSION 1.1.0
    LANGUAGES CXX
)

set(PROJECT_AUTHOR "Anirudh Mathur")
set(PROJECT_HOMEPAGE "https://github.com/AnirudhMathur12/cappuccino")

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Check if working directory is dirty
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_IS_DIRTY
        ERROR_QUIET
    )
    
    if(GIT_IS_DIRTY)
        set(GIT_COMMIT_HASH "${GIT_COMMIT_HASH}-dirty")
    endif()
else()
    set(GIT_COMMIT_HASH "unknown")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE)
    message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ON)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    main.cpp
    utils.cpp
    Token.cpp
    AbstractSyntaxTree.cpp
    CodeGen.cpp
    Parser.cpp
)

set(HEADERS
        utils.h
        Token.h
        AbstractSyntaxTree.h
        Parser.h
        CodeGen.h
        capp_stdlib.h
)

configure_file(
    "${PROJECT_SOURCE_DIR}/version.h.in"
    "${PROJECT_BINARY_DIR}/version.h"
    @ONLY
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
)

if(APPLE)
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        message(FATAL_ERROR 
            "This compiler only supports Apple Silicon (M-series) Macs.\n"
            "Current architecture: ${CMAKE_SYSTEM_PROCESSOR}\n"
            "Please use an M1, M2, M3, or newer Mac."
        )
    endif()
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_MACOS)
    message(STATUS "Building for Apple Silicon (ARM64)")
else()
    message(FATAL_ERROR 
        "This compiler only supports macOS on Apple Silicon.\n"
        "Current platform: ${CMAKE_SYSTEM_NAME}\n"
        "Please use an M-series Mac."
    )
endif()

install(TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:        ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "System:          ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture:    ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "")
