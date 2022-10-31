cmake_minimum_required(VERSION 3.21)

# Enable policy to make vcpkg copy dll/so to build directory
cmake_policy(SET CMP0087 NEW)

# include logic to build glsl files
include(cmake/glsl_compiler.cmake)

# project and language to be used
project(vulkan-eg
	LANGUAGES CXX)

# ensure project executables get placed in this path,
# required by glsl compiler
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin/")

# main executable source folder
add_subdirectory(src)