#include "renderer.h"

using namespace vulkan_eg;
using namespace std::string_literals;

namespace 
{
	constexpr auto wanted_layers = std::array{
#ifdef _DEBUG
		"VK_LAYER_KHRONOS_validation",
#endif
	};

	constexpr auto wanted_extensions = std::array{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
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

	template <size_t S>
	auto find_best_extensions(const std::array<const char *, S> &wanted, 
	                          const std::vector<vk::ExtensionProperties> &installed)
	  -> std::vector<const char *>
	{
		auto out = std::vector<const char *>{};
		for (auto &&w : wanted)
		{
			for (auto &&i : installed)
			{
				if (std::string(i.extensionName.data()).compare(w) == 0)
				{
					out.emplace_back(w);
					break;
				}
			}
		}

		return out;
	}

	template <size_t S>
	auto find_best_layers(const std::array<const char *, S> &wanted, 
	                      const std::vector<vk::LayerProperties> &installed)
	  -> std::vector<const char *>
	{
		auto out = std::vector<const char *>{};
		for (auto &&w : wanted)
		{
			for (auto &&i : installed)
			{
				if (std::string(i.layerName.data()).compare(w) == 0)
				{
					out.emplace_back(w);
					break;
				}
			}
		}
		return out;
	}

	auto find_queue_family(vk::PhysicalDevice &device) -> std::optional<std::pair<uint32_t, vk::QueueFamilyProperties>>
	{
		auto queue_families = device.getQueueFamilyProperties();
		auto queue_family_iter = std::ranges::find_if(queue_families, [](vk::QueueFamilyProperties &qf) -> bool{
			return static_cast<bool>(qf.queueFlags & vk::QueueFlagBits::eGraphics);
		});

		if (queue_family_iter == queue_families.end())
		{
			return std::nullopt;
		}

		auto queue_index = std::distance(queue_families.begin(), queue_family_iter);

		return std::pair{
			static_cast<uint32_t>(queue_index),
			*queue_family_iter
		};
	}

	auto find_suitable_device(vk::Instance &instance) -> std::optional<vk::PhysicalDevice>
	{
		auto devices = instance.enumeratePhysicalDevices();

		auto device_iter = std::ranges::find_if(devices, [](vk::PhysicalDevice &device)
		{
			return find_queue_family(device).has_value();
		});

		if (device_iter == devices.end())
		{
			return std::nullopt;
		}

		return *device_iter;
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

renderer::renderer(HWND windowHandle)
{

	auto name = get_window_name(windowHandle);

	create_vulkan_instance(name);
#ifdef _DEBUG
	setup_debug_callback();
#endif
	pick_physical_device();
	create_logical_device();
}

renderer::~renderer()
{
	device.destroy();

#ifdef _DEBUG
	instance.destroyDebugUtilsMessengerEXT(debug_messenger);
#endif
	instance.destroy();
}

void renderer::create_vulkan_instance(std::string_view name)
{
	try
	{	
		auto appInfo = vk::ApplicationInfo{} = {
			.pApplicationName = name.data(),
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = name.data(),
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_3
		};

		auto installed_extensions = vk::enumerateInstanceExtensionProperties();
		auto extensions = find_best_extensions(wanted_extensions, installed_extensions);

		auto installed_layers = vk::enumerateInstanceLayerProperties();
		auto layers = find_best_layers(wanted_layers, installed_layers);

		auto createInfo = vk::InstanceCreateInfo(
			vk::InstanceCreateFlags{}, 
			&appInfo,
			layers, 
			extensions 
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

void renderer::setup_debug_callback()
{
	auto severityFlags = vk::DebugUtilsMessageSeverityFlagsEXT(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
	                                                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError );
	auto messageTypeFlags = vk::DebugUtilsMessageTypeFlagsEXT(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
	                                                          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
	                                                          vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
	auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT( {}, severityFlags, messageTypeFlags, &debug_callback );

	debug_messenger = instance.createDebugUtilsMessengerEXT(createInfo);
}

void renderer::pick_physical_device()
{
	if (auto suitable_device = find_suitable_device(instance);
	    suitable_device.has_value())
	{
		physical_device = suitable_device.value();
	}
	else
	{
		throw std::runtime_error("No suitable device found.");
	}
}

void renderer::create_logical_device()
{
	auto queue_family = find_queue_family(physical_device).value();

	auto queue_priority = 1.0f;
	auto queue_createInfo = vk::DeviceQueueCreateInfo({}, static_cast<uint32_t>(queue_family.first), 1, &queue_priority);

	auto queue_array = std::vector{queue_createInfo};
	auto layers = find_best_layers(wanted_layers, vk::enumerateInstanceLayerProperties());
	auto extensions = find_best_extensions(wanted_extensions, vk::enumerateInstanceExtensionProperties());
	auto device_features = vk::PhysicalDeviceFeatures{};

	auto device_createInfo = vk::DeviceCreateInfo({}, queue_array, layers, /*extensions*/{});
	device = physical_device.createDevice(device_createInfo);
}