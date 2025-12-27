cmake_minimum_required(VERSION 3.20)
project(tide VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================================
# C++ Standard Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Platform Check - Linux Only
# ============================================================================
if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "tide is a Linux-only terminal emulator")
endif()

# ============================================================================
# Dependencies
# ============================================================================

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW
find_package(glfw3 3.3 REQUIRED)

# FreeType
find_package(Freetype REQUIRED)

# ============================================================================
# Source Files
# ============================================================================

set(CORE_SOURCES
    core/pty.cpp
    core/ansi_parser.cpp
    core/grid_buffer.cpp
)

set(RENDER_SOURCES
    render/renderer.cpp
    render/font.cpp
)

set(INPUT_SOURCES
    input/keyboard.cpp
    input/mouse.cpp
)

set(THEME_SOURCES
    theme/theme.cpp
)

set(PLATFORM_SOURCES
    platform/linux/window.cpp
)

set(GLAD_SOURCES
    glad/glad.c
)

set(APP_SOURCES
    app/main.cpp
)

# ============================================================================
# Executable
# ============================================================================

add_executable(${PROJECT_NAME}
    ${CORE_SOURCES}
    ${RENDER_SOURCES}
    ${INPUT_SOURCES}
    ${THEME_SOURCES}
    ${PLATFORM_SOURCES}
    ${GLAD_SOURCES}
    ${APP_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    Freetype::Freetype
    util  # For forkpty() on Linux
)

# ============================================================================
# Installation (optional, for future use)
# ============================================================================
# TODO: Add install rules when ready for distribution
