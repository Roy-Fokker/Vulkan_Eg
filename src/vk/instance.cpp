#include "instance.hpp"

using namespace vulkan_eg::vkw;

namespace
{
	static auto wanted_instance_extensions = []{
		auto exts = std::vector<std::string>
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		#ifdef _DEBUG
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		#endif
		};
		std::ranges::sort(exts);
		return exts;
	}();

	static auto wanted_instance_layers = []{
		auto lyrs = std::vector<std::string>
		{
		#ifdef _DEBUG
			"VK_LAYER_KHRONOS_validation",
		#endif
		};
		std::ranges::sort(lyrs);
		return lyrs;
	}();

	auto get_installed_extensions() -> std::vector<std::string>
	{
		auto exts = vk::enumerateInstanceExtensionProperties();
		auto out = std::vector<std::string>{};

		std::ranges::transform(exts, std::back_inserter(out), [](vk::ExtensionProperties props)
		{
			return std::string(props.extensionName.data());
		});

		std::ranges::sort(out);

		return out;
	}

	auto get_installed_layers() -> std::vector<std::string>
	{
		auto lyrs = vk::enumerateInstanceLayerProperties();
		auto out = std::vector<std::string>{};

		std::ranges::transform(lyrs, std::back_inserter(out), [](vk::LayerProperties props)
		{
			return std::string(props.layerName.data());
		});

		std::ranges::sort(out);

		return out;
	}

	auto convert_to_vec_char(std::vector<std::string> &in) -> std::vector<const char *>
	{
		auto out = std::vector<const char *>{};
		std::ranges::transform(in, std::back_inserter(out), [](std::string &s)
		{
			auto c_str = new char[s.size()];
			std::strcpy(c_str, s.c_str());
			return c_str;
		});
		return out;
	}
}

#ifdef _DEBUG
static auto debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                           void *pUserData)
-> vk::Bool32
{
	if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{
		std::cerr << std::format("Validation Layer: {}\n", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

static auto vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                                           const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
-> VkResult
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, 
                                            VkDebugUtilsMessengerEXT debugMessenger, 
                                            const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

#endif

instance::instance(std::string_view name, std::string_view engine, uint32_t version, HWND window_handle)
{
	create_instance(name, engine, version);

#ifdef _DEBUG
	setup_debug_callback();
#endif
	create_surface(window_handle);
}

instance::~instance()
{
	vk_instance.destroySurfaceKHR(vk_surface);
	vk_surface = nullptr;

#ifdef _DEBUG
	vk_instance.destroyDebugUtilsMessengerEXT(debug_messenger);
	debug_messenger = nullptr;
#endif

 	vk_instance.destroy();
	vk_instance = nullptr;
}

void instance::create_instance(std::string_view name, std::string_view engine, uint32_t version)
{
	auto app_info = vk::ApplicationInfo
	{
		.pApplicationName = name.data(),
		.applicationVersion = version,
		.pEngineName = engine.data(),
		.engineVersion = version,
		.apiVersion = VK_API_VERSION_1_3
	};

	auto installed_extensions = get_installed_extensions();
	auto available_extensions = std::vector<std::string>{};
	std::ranges::set_intersection(installed_extensions, wanted_instance_extensions, std::back_inserter(available_extensions));
	auto exts = convert_to_vec_char(available_extensions);

	auto lyrs = get_layers();

	auto create_info = vk::InstanceCreateInfo
	{
		.pApplicationInfo = &app_info,
		.enabledLayerCount = static_cast<uint32_t>(lyrs.size()),
		.ppEnabledLayerNames = lyrs.data(),
		.enabledExtensionCount = static_cast<uint32_t>(exts.size()),
		.ppEnabledExtensionNames = exts.data()
	};

	try 
	{
		vk_instance = vk::createInstance(create_info);
	}
	catch(vk::SystemError &err)
	{
		std::cerr << std::format("Vulkan System Error: {}\n", err.what());
		throw err;
	}
}

void instance::setup_debug_callback()
{
	auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT
	{
		.messageSeverity = {
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
		}, 
		.messageType = {
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		}, 
		.pfnUserCallback = &debug_callback
	};

	debug_messenger = vk_instance.createDebugUtilsMessengerEXT(createInfo);
}

void instance::create_surface(HWND window_handle)
{
	auto create_info = vk::Win32SurfaceCreateInfoKHR
	{
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = window_handle
	};

	vk_surface = vk_instance.createWin32SurfaceKHR(create_info);
}

auto instance::get() const -> std::tuple<const vk::Instance &, const vk::SurfaceKHR &>
{
	return 
	{
		vk_instance, 
		vk_surface
	};
}

auto instance::get_layers() const -> std::vector<const char *>
{
	auto installed_layers = get_installed_layers();
	auto available_layers = std::vector<std::string>{};
	std::ranges::set_intersection(installed_layers, wanted_instance_layers, std::back_inserter(available_layers));
	auto lyrs = convert_to_vec_char(available_layers);

	return lyrs;
}