#include "renderer.h"

using namespace vulkan_eg;
using namespace std::string_literals;

namespace 
{
	constexpr auto validation_layers = std::array{
		"VK_LAYER_KHRONOS_validation",
	};

	constexpr auto extension_names = std::array{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		"VK_KHR_win32_surface",
	};

	auto get_window_name(HWND handle) -> std::string
	{
		auto len = GetWindowTextLengthA(handle) + 1;
		auto name{""s};
		name.resize(len);
		auto result = GetWindowTextA(handle, &name[0], len);
		name.resize(len - 1);

		return name;
	}
}

renderer::renderer(HWND windowHandle)
{
	auto name = get_window_name(windowHandle);

	create_vulkan_instance(name);
}

renderer::~renderer() = default;

void renderer::create_vulkan_instance(std::string_view name)
{
	try
	{	
		auto appInfo = vk::ApplicationInfo(
			name.data(),
			VK_MAKE_VERSION(1, 0, 0),
			name.data(),
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_3
		);

		auto createInfo = vk::InstanceCreateInfo(
			{}, 
			&appInfo,
			validation_layers, 
			extension_names
		);

		instance = vk::createInstance(createInfo);
	}
	catch (vk::SystemError &err)
	{
		std::cout << std::format("System Error: {}\n", err.what());
		throw err;
	}
	catch (std::exception &err)
	{
		std::cout << std::format("Standard Exception: {}\n", err.what());
		throw err;
	}
}