cmake_minimum_required(VERSION 3.21)

# Enable policy to make vcpkg copy dll/so to build directory
cmake_policy(SET CMP0087 NEW)

# project and language to be used
project(vulkan-eg
	LANGUAGES CXX)

# main executable source folder
add_subdirectory(src)