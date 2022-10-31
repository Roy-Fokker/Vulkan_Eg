# Compile glsl files into SPIR-V files
# depends on glslc installed by LunarG SDK

# Usage: target_shader_sources(<target> [<file> ...])
function (target_shader_sources TARGET)
	find_package(Vulkan REQUIRED)
	if (NOT TARGET Vulkan::glslc)
		message(FATAL_ERROR "[Error]: Could not find glslc.")
	endif()

	foreach(source IN LISTS ARGN)
		get_filename_component(source_fldr ${source} DIRECTORY)
		get_filename_component(source_abs ${source} ABSOLUTE)
		get_filename_component(basename ${source_abs} NAME)

		set(shader_dir ${EXECUTABLE_OUTPUT_PATH}/${source_fldr})
		set(output ${shader_dir}/${basename}.spv)
		
		if(NOT EXISTS ${source_abs})
			message(FATAL_ERROR "Cannot file shader file: ${source}")
		endif()

		add_custom_command(
			OUTPUT ${output}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${shader_dir}
			COMMAND Vulkan::glslc ${source_abs} -o ${output}
			DEPENDS ${source_abs}
			COMMENT "Compiling SPIRV: ${source} -> ${output}"
			VERBATIM
		)

		set(shader_target "${TARGET}_${basename}")
		add_custom_target("${shader_target}"
		                  DEPENDS "${output}")
		add_dependencies("${TARGET}" "${shader_target}")
	endforeach()

endfunction()