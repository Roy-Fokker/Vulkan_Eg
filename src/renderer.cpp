#include "renderer.h"

using namespace vulkan_eg;
using namespace std::string_literals;

namespace 
{
	constexpr auto wanted_instance_layers = std::array{
#ifdef _DEBUG
		"VK_LAYER_KHRONOS_validation",
#endif
	};

	constexpr auto wanted_instance_extensions = std::array{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
	};

	constexpr auto wanted_device_extensions = std::array{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	auto get_window_name(HWND handle) -> std::string
	{
		auto len = static_cast<size_t>(GetWindowTextLengthA(handle)) + 1;
		auto name{""s};
		name.resize(len);
		auto result = GetWindowTextA(handle, &name[0], static_cast<int>(len));
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

	struct queue_family
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		auto is_complete() -> bool
		{
			return graphics_family.has_value() 
			   and present_family.has_value();
		}
	};

	auto find_queue_family(vk::PhysicalDevice &device, vk::SurfaceKHR &surface) -> queue_family
	{
		auto out = queue_family{};

		auto queue_families = device.getQueueFamilyProperties();
		
		auto queue_family_iter = std::ranges::find_if(queue_families, [&](vk::QueueFamilyProperties &qf) -> bool
		{
			return static_cast<bool>(qf.queueFlags & vk::QueueFlagBits::eGraphics);
		});

		if (queue_family_iter != queue_families.end())
		{
			out.graphics_family = static_cast<uint32_t>(std::distance(queue_families.begin(), queue_family_iter));
		}

		auto queue_idx{0};
		queue_family_iter = std::ranges::find_if(queue_families, [&](vk::QueueFamilyProperties &qf) -> bool
		{
			auto present_support = device.getSurfaceSupportKHR(queue_idx, surface);
			queue_idx++;

			return static_cast<bool>(present_support);
		});

		if (queue_family_iter != queue_families.end())
		{
			out.present_family = static_cast<uint32_t>(std::distance(queue_families.begin(), queue_family_iter));
		}
		
		return out;
	}

	auto check_device_extension_support(vk::PhysicalDevice &device) -> bool
	{
		auto available_extensions = device.enumerateDeviceExtensionProperties();
		auto supported = find_best_extensions(wanted_device_extensions, available_extensions);

		return (supported.size() == wanted_device_extensions.size());
	}

	struct swap_chain_support_details
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> present_modes;
	};

	auto query_swap_chain_support(vk::PhysicalDevice &device, vk::SurfaceKHR &surface) -> swap_chain_support_details
	{
		return swap_chain_support_details{
			.capabilities = device.getSurfaceCapabilitiesKHR(surface),
			.formats = device.getSurfaceFormatsKHR(surface),
			.present_modes = device.getSurfacePresentModesKHR(surface),
		};
	}

	auto find_suitable_device(vk::Instance &instance, vk::SurfaceKHR &surface) -> std::optional<vk::PhysicalDevice>
	{
		auto devices = instance.enumeratePhysicalDevices();

		auto device_iter = std::ranges::find_if(devices, [&](vk::PhysicalDevice &device)
		{
			auto scsd = query_swap_chain_support(device, surface);

			return find_queue_family(device, surface).is_complete()
				and check_device_extension_support(device)
				and not scsd.formats.empty() 
				and not scsd.present_modes.empty();
		});

		if (device_iter == devices.end())
		{
			return std::nullopt;
		}

		return *device_iter;
	}

	auto choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR> &available_formats) -> vk::SurfaceFormatKHR
	{
		auto format_iter = std::ranges::find_if(available_formats, [](const vk::SurfaceFormatKHR &av)
		{
			return av.format == vk::Format::eB8G8R8A8Srgb
				and av.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});

		if (format_iter == available_formats.end())
		{
			throw std::runtime_error("Unable to find desired surface format");
		}

		return *format_iter;
	}

	auto choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &available_present_modes) -> vk::PresentModeKHR
	{
		auto present_mode_iter = std::ranges::find_if(available_present_modes, [](const vk::PresentModeKHR &pm)
		{
			return pm == vk::PresentModeKHR::eMailbox;
		});

		if (present_mode_iter == available_present_modes.end())
		{
			return vk::PresentModeKHR::eFifo;
		}
		
		return *present_mode_iter;
	}

	auto choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities) -> vk::Extent2D
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		throw std::runtime_error("Current Extent width exceeds numeric max.");
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
	create_surface(windowHandle);
	pick_physical_device();
	create_logical_device();
	create_swap_chain();
}

renderer::~renderer()
{
	device.destroySwapchainKHR(swap_chain);
	device.destroy();

	instance.destroySurfaceKHR(surface);
#ifdef _DEBUG
	instance.destroyDebugUtilsMessengerEXT(debug_messenger);
#endif
	instance.destroy();
}

void renderer::create_vulkan_instance(std::string_view name)
{
	try
	{	
		auto appInfo = vk::ApplicationInfo
		{
			.pApplicationName = name.data(),
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = name.data(),
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_3
		};

		auto installed_extensions = vk::enumerateInstanceExtensionProperties();
		auto extensions = find_best_extensions(wanted_instance_extensions, installed_extensions);

		auto installed_layers = vk::enumerateInstanceLayerProperties();
		auto layers = find_best_layers(wanted_instance_layers, installed_layers);

		auto createInfo = vk::InstanceCreateInfo
		{
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(layers.size()),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()
		};

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
	auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT
	{
		.messageSeverity = severityFlags, 
		.messageType = messageTypeFlags, 
		.pfnUserCallback = &debug_callback
	};

	debug_messenger = instance.createDebugUtilsMessengerEXT(createInfo);
}

void renderer::create_surface(HWND windowHandle)
{
	auto createInfo = vk::Win32SurfaceCreateInfoKHR
	{
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = windowHandle
	};

	surface = instance.createWin32SurfaceKHR(createInfo);
}

void renderer::pick_physical_device()
{
	if (auto suitable_device = find_suitable_device(instance, surface);
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
	auto qf = find_queue_family(physical_device, surface);

	auto queue_priority = 1.0f;
	auto queue_array = std::vector
	{
		vk::DeviceQueueCreateInfo
		{
			.queueFamilyIndex = static_cast<uint32_t>(qf.graphics_family.value()),
			.queueCount = 1,
			.pQueuePriorities = &queue_priority
		},
		vk::DeviceQueueCreateInfo
		{
			.queueFamilyIndex = static_cast<uint32_t>(qf.present_family.value()),
			.queueCount = 1,
			.pQueuePriorities = &queue_priority
		},
	};
	auto layers = find_best_layers(wanted_instance_layers, vk::enumerateInstanceLayerProperties());
	auto extensions = find_best_extensions(wanted_device_extensions, physical_device.enumerateDeviceExtensionProperties());
	auto device_features = vk::PhysicalDeviceFeatures{};

	auto device_createInfo = vk::DeviceCreateInfo
	{
		.queueCreateInfoCount = static_cast<uint32_t>(queue_array.size()),
		.pQueueCreateInfos = queue_array.data(),
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),
		.ppEnabledLayerNames = layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};
	device = physical_device.createDevice(device_createInfo);

	graphics_queue = device.getQueue(qf.graphics_family.value(), 0);
	present_queue = device.getQueue(qf.present_family.value(), 0);
}

void renderer::create_swap_chain()
{
	auto scs = query_swap_chain_support(physical_device, surface);
	auto sf = choose_swap_surface_format(scs.formats);
	auto pm = choose_swap_present_mode(scs.present_modes);
	auto extent = choose_swap_extent(scs.capabilities);

	auto image_count = std::clamp(0u, scs.capabilities.minImageCount + 1, scs.capabilities.maxImageCount);

	auto qf = find_queue_family(physical_device, surface);
	auto ism = (qf.graphics_family != qf.present_family)
	         ? vk::SharingMode::eConcurrent
	         : vk::SharingMode::eExclusive;
	auto qfl = (qf.graphics_family != qf.present_family)
	         ? std::vector{qf.graphics_family.value(), qf.present_family.value()}
	         : std::vector<uint32_t>{};

	auto createInfo = vk::SwapchainCreateInfoKHR
	{
		.surface = surface, 
		.minImageCount = image_count,
		.imageFormat = sf.format,
		.imageColorSpace = sf.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.imageSharingMode = ism,
		.queueFamilyIndexCount = static_cast<uint32_t>(qfl.size()),
		.pQueueFamilyIndices = qfl.data(),
		.preTransform = scs.capabilities.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = pm,
		.clipped = true
	};

	swap_chain = device.createSwapchainKHR(createInfo);
}