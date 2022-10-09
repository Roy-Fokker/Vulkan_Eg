#include "renderer.h"

using namespace vulkan_eg;

renderer::renderer(std::string_view name)
{
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

		auto layer_names = std::vector{
			"VK_LAYER_KHRONOS_validation",
		};

		auto extension_names = std::vector<const char *>{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			"VK_KHR_win32_surface",
		};

		auto createInfo = vk::InstanceCreateInfo(
			{}, 
			&appInfo,
			layer_names, 
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