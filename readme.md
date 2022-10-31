# Vulkan Example

Using C++20 on Windows

---
## Dependencies
- vcpkg
	- Vulkan
	- GLM
- cmake
- LunarG's Vulkan SDK

---
## CMake Vulkan::GLSLC caveats
- requires `EXECUTABLE_OUTPUT_PATH` to be defined
- must include `cmake/glsl_compiler.cmake` which has function `target_shader_sources`
- for VSCode to ensure debugger (F5) launches in correct folder with vscode-cmaketools `v1.12.27`
	- assume `EXECUTABLE_OUTPUT_PATH` is set to `${CMAKE_BINARY_DIR}/bin/`
	- then must set `cwd` in `launch.json` to `${workspaceRoot}/builds/${command:cmake.activeConfigurePresetName}/bin`
	- This *ONLY* works with F5, does not work debug icon/button on status bar.

---
## References
- https://vulkan-tutorial.com/
