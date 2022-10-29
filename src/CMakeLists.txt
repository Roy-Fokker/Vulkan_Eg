# find all libraries defined in vcpkg
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

# find paths for header only libraries
#find_path(VULKAN_HPP_INCLUDE_DIRS "vulkan/vulkan.hpp")

# update executable name
add_executable(vulkan-eg)

# set C++ standard to use
target_compile_features(vulkan-eg
	PRIVATE 
		cxx_std_20)

# set preprocessor defines for executable
target_compile_definitions(vulkan-eg
	PRIVATE
		UNICODE _UNICODE 
		_SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING 
		NOMINMAX 
		WIN32_LEAN_AND_MEAN
		VK_USE_PLATFORM_WIN32_KHR
		VULKAN_HPP_NO_CONSTRUCTORS)

# executable specific target options
target_link_options(vulkan-eg
	PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/entry:mainCRTStartup>)

# additional include directories
target_include_directories(vulkan-eg
	PRIVATE
		# ${VULKAN_HPP_INCLUDE_DIRS}
		)

# libraries that are used by executable
target_link_libraries(vulkan-eg
	PRIVATE
		Vulkan::Vulkan
		glm::glm
		range-v3)

# Use Precompiled headers for std/os stuff
target_precompile_headers(vulkan-eg
	PRIVATE
		pch.h)

# sources to be used
target_sources(vulkan-eg
	PRIVATE
		main.cpp
		window.cpp
		renderer.cpp)