# Three arguments can be passed:
# the path of the project to build [default is sine example]
# the name of the bin [default is name of default project]
# debug flag [default is 0]
# like this:
# cmake -DPRJ_PATH=path/to/project -DBIN_NAME=name -DDEBUG=1
# notice that the project path must be relative to AudioEngine's root
# i.e., relative to where this CMakeLists.txt file is

cmake_minimum_required(VERSION 3.5)
project(AudioEngine)

# Option to control whether a project should be built
option(BUILD_PROJECT "Build project" ON)

set(DEFAULT_PRJ "examples/renderBased/sine")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEBUG)
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall -Wextra")
else()
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -Wall -Wextra")
endif()

# Set the output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories(${INCLUDE_DIRS})

# Core files
file(GLOB_RECURSE SRC_FILES "core/*.cpp")

# Conditionally include and build the specified project
if(BUILD_PROJECT)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # Assign default project if one is not specified
    if(NOT PRJ_PATH)
        set(PRJ_PATH ${DEFAULT_PRJ})
    endif()

    # Get project source files
    file(GLOB_RECURSE PRJ_SRC_FILES "${PRJ_PATH}/*.cpp")

    # Check if project has render.cpp
    set(HAS_RENDER_CPP OFF)
    foreach(FILE ${PRJ_SRC_FILES})
        if(FILE MATCHES ".*/render\\.cpp$")
            set(HAS_RENDER_CPP ON)
            break()
        endif()
    endforeach()

    # Handle default_render.cpp based on presence of render.cpp
    if(HAS_RENDER_CPP)
        message(STATUS "User-defined render.cpp found. Excluding default_render.cpp.")
        list(FILTER SRC_FILES EXCLUDE REGEX ".*core/default_render\\.cpp$")
    else()
        message(STATUS "No render.cpp found in project. Using default_render.cpp.")
        add_definitions(-DDEFAULT_RENDER)
    endif()

    # Combine project source files with core source files
    list(APPEND ALL_SRC_FILES ${SRC_FILES})
    list(APPEND ALL_SRC_FILES ${PRJ_SRC_FILES})

    # Determine binary name if not specified
    if(NOT BIN_NAME)
        get_filename_component(BIN_NAME ${PRJ_PATH} NAME)
    endif()

    add_executable(${BIN_NAME} ${ALL_SRC_FILES})

    # Link necessary libraries
    target_link_libraries(${BIN_NAME} -pthread -ffast-math m asound sndfile fftw3f fftw3)
else() # when engine is used as library by a parent project
    # Expose the core source and include files to the 'upmodule' (parent project)
    set(ENGINE_SRC_FILES ${SRC_FILES} PARENT_SCOPE)
    set(ENGINE_INCLUDE_DIRS ${INCLUDE_DIRS} PARENT_SCOPE)

    # Add DEFAULT_RENDER definition for core-only build
    add_definitions(-DDEFAULT_RENDER)
endif()
