#include "devices.hpp"

#include "instance.hpp"
#include "swap_chain.hpp"

using namespace vulkan_eg::vkw;

namespace
{
	const auto wanted_device_extensions = std::vector
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	auto check_device_extension_support(const vk::PhysicalDevice &device, const std::vector<const char *> &extensions) -> bool
	{
		auto device_exts = device.enumerateDeviceExtensionProperties(); 
		auto trans_view = device_exts
		                | std::views::transform([](const vk::ExtensionProperties &prop) -> const char *
		{
			return prop.extensionName.data();
		});
		auto supported_extensions = std::vector(std::begin(trans_view), std::end(trans_view));
		std::ranges::sort(supported_extensions, [](const char *a, const char *b)
		{
			return std::string_view(a) < std::string_view(b);
		});

		auto intersection = std::vector<const char *>{};
		std::ranges::set_intersection(extensions, supported_extensions, std::back_inserter(intersection), [](const char *a, const char *b)
		{
			return std::string_view(a) == std::string_view(b);
		});

		return (intersection.size() == extensions.size());
	}

	auto find_queue_family(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface) -> queue_family
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
}

auto queue_family::is_complete() const -> bool
{
	return graphics_family.has_value() 
		and present_family.has_value();
}

auto queue_family::get_array() const -> std::vector<vk::DeviceQueueCreateInfo>
{
	auto out = std::vector<vk::DeviceQueueCreateInfo>{};
	auto queue_priority = 1.0f;

	if (graphics_family.has_value())
	{
		out.emplace_back(vk::DeviceQueueCreateInfo
		{
			.queueFamilyIndex = static_cast<uint32_t>(graphics_family.value()),
			.queueCount = 1,
			.pQueuePriorities = &queue_priority
		});
	}

	if (present_family.has_value() and present_family.value() != graphics_family.value())
	{
		out.emplace_back(vk::DeviceQueueCreateInfo
		{
			.queueFamilyIndex = static_cast<uint32_t>(present_family.value()),
			.queueCount = 1,
			.pQueuePriorities = &queue_priority
		});
	}

	return out;
}

devices::devices(const instance *vkw_inst)
{
	pick_physical_device(vkw_inst);
	create_logical_device(vkw_inst);
}

devices::~devices()
{
	vk_logical_device.destroy();
	vk_logical_device = nullptr;
}

void devices::pick_physical_device(const instance *vkw_inst)
{
	auto &&[instance, surface] = vkw_inst->get();

	auto physical_devices = instance.enumeratePhysicalDevices();
	auto suitable_device_iter = std::ranges::find_if(physical_devices, [&](vk::PhysicalDevice &device)
	{
		auto srfc_dtls = query_surface_details(device, surface);
		auto exts_supported = check_device_extension_support(device, wanted_device_extensions);
		auto que_fam = find_queue_family(device, surface);

		return que_fam.is_complete()
		   and exts_supported
		   and not srfc_dtls.formats.empty()
		   and not srfc_dtls.present_modes.empty();
	});
	if (suitable_device_iter == physical_devices.end())
	{
		throw std::runtime_error("Cannot find suitable physical device.");
	}
	vk_physical_device = *suitable_device_iter;
}

void devices::create_logical_device(const instance *vkw_inst)
{
	auto &&[instance, surface] = vkw_inst->get();
	qf = find_queue_family(vk_physical_device, surface);

	auto queue_array = qf.get_array();

	auto layers = vkw_inst->get_layers();
	auto extensions = wanted_device_extensions;
	auto device_features = vk::PhysicalDeviceFeatures{};

	auto device_createInfo = vk::DeviceCreateInfo
	{
		.queueCreateInfoCount = static_cast<uint32_t>(queue_array.size()),
		.pQueueCreateInfos = queue_array.data(),
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),
		.ppEnabledLayerNames = layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &device_features
	};

	vk_logical_device = vk_physical_device.createDevice(device_createInfo);

	vk_graphics_queue = vk_logical_device.getQueue(qf.graphics_family.value(), 0);
	vk_present_queue = vk_logical_device.getQueue(qf.present_family.value(), 0);
}

auto devices::get_queue_family() const -> queue_family
{
	return qf;
}

auto devices::get_device() -> vk::Device &
{
	return vk_logical_device;
}

auto devices::get_physical_device() -> vk::PhysicalDevice &
{
	return vk_physical_device;
}

auto devices::get_queues() -> std::tuple<vk::Queue &, vk::Queue &>
{
	return
	{
		vk_graphics_queue,
		vk_present_queue
	};
}