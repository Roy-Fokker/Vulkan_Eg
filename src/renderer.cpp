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

	auto read_file(const std::filesystem::path &filename) -> std::vector<uint32_t>
	{
		auto file = std::ifstream(filename, std::ios::ate | std::ios::binary);

		if (not file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		auto file_size = file.tellg();
		auto buffer = std::vector<uint32_t>(file_size);
		
		file.seekg(0);
		file.read(reinterpret_cast<char *>(buffer.data()), file_size);

		file.close();

		return buffer;
	}

	auto create_shader_module(vk::Device &device, const std::vector<uint32_t> &shader_code) -> vk::ShaderModule
	{
		auto createInfo = vk::ShaderModuleCreateInfo
		{
			.codeSize = shader_code.size(),
			.pCode = shader_code.data()
		};

		return device.createShaderModule(createInfo);
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
	create_image_views();
	create_render_pass();
	create_graphics_pipeline();

	create_frame_buffers();
	create_command_pool();
	create_command_buffer();
}

renderer::~renderer()
{
	device.destroyCommandPool(command_pool);

	for (auto &fb : swap_chain_frame_buffers)
	{
		device.destroyFramebuffer(fb);
	}

	device.destroyPipeline(graphics_pipeline);
	device.destroyPipelineLayout(pipeline_layout);

	device.destroyRenderPass(render_pass);

	for (auto &iv : swap_chain_image_views)
	{
		device.destroyImageView(iv);
	}

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

	swap_chain_images = device.getSwapchainImagesKHR(swap_chain);
	swap_chain_format = sf.format;
	swap_chain_extent = extent;
}

void renderer::create_image_views()
{
	swap_chain_image_views.resize(swap_chain_images.size());
	for(auto&& [swap_chain_image, swap_chain_image_view] : ranges::views::zip(swap_chain_images, swap_chain_image_views))
	{
		auto createInfo = vk::ImageViewCreateInfo
		{
			.image = swap_chain_image,
			.viewType = vk::ImageViewType::e2D,
			.format = swap_chain_format,
			.components = {
				.r = vk::ComponentSwizzle::eIdentity,
				.g = vk::ComponentSwizzle::eIdentity,
				.b = vk::ComponentSwizzle::eIdentity,
				.a = vk::ComponentSwizzle::eIdentity,
			},
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		swap_chain_image_view = device.createImageView(createInfo);
	}
}

void renderer::create_render_pass()
{
	auto color_attachment = vk::AttachmentDescription
	{
		.format = swap_chain_format,
		.samples = vk::SampleCountFlagBits::e1,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::ePresentSrcKHR
	};

	auto color_attachment_ref = vk::AttachmentReference
	{
		.attachment = 0,
		.layout = vk::ImageLayout::eColorAttachmentOptimal
	};

	auto sub_pass = vk::SubpassDescription
	{
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref
	};

	auto render_pass_ci = vk::RenderPassCreateInfo
	{
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &sub_pass
	};

	render_pass = device.createRenderPass(render_pass_ci);
}

void renderer::create_graphics_pipeline()
{
	auto vert_shader_file = read_file("shaders/shader.vert.spv");
	auto frag_shader_file = read_file("shaders/shader.frag.spv");

	auto vert_shader = create_shader_module(device, vert_shader_file);
	auto frag_shader = create_shader_module(device, frag_shader_file);

	auto vert_shdr_ci = vk::PipelineShaderStageCreateInfo
	{
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = vert_shader,
		.pName = "main"
	};

	auto frag_shdr_ci = vk::PipelineShaderStageCreateInfo
	{
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = frag_shader,
		.pName = "main"
	};

	auto shader_stages = std::vector
	{
		vert_shdr_ci, 
		frag_shdr_ci
	};

	auto vert_input_ci = vk::PipelineVertexInputStateCreateInfo
	{
		.vertexBindingDescriptionCount = 0,
		.vertexAttributeDescriptionCount = 0
	};

	auto inpt_asmbly_ci = vk::PipelineInputAssemblyStateCreateInfo
	{
		.topology = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = false
	};

	auto viewport_ci = vk::PipelineViewportStateCreateInfo
	{
		.viewportCount = 1,
		.scissorCount = 1
	};

	auto rasterizer_ci = vk::PipelineRasterizationStateCreateInfo
	{
		.depthClampEnable = false,
		.rasterizerDiscardEnable = false,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eClockwise,
		.depthBiasEnable = false,
		.lineWidth = 1.0f
	};

	auto multisample_ci = vk::PipelineMultisampleStateCreateInfo
	{
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable = false
	};

	auto clr_blend_attch_st = vk::PipelineColorBlendAttachmentState
	{
		.blendEnable = false,
		.colorWriteMask = vk::ColorComponentFlagBits::eR
		                | vk::ColorComponentFlagBits::eG
		                | vk::ColorComponentFlagBits::eB
		                | vk::ColorComponentFlagBits::eA
	};

	auto color_blend_ci = vk::PipelineColorBlendStateCreateInfo
	{
		.logicOpEnable = false,
		.logicOp = vk::LogicOp::eCopy,
		.attachmentCount = 1,
		.pAttachments = &clr_blend_attch_st,
		.blendConstants = std::array{0.0f, 0.0f, 0.0f, 0.0f}
	};

	auto dynamic_states_array = std::vector
	{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	auto dynamic_state = vk::PipelineDynamicStateCreateInfo
	{
		.dynamicStateCount = static_cast<uint32_t>(dynamic_states_array.size()),
		.pDynamicStates = dynamic_states_array.data()
	};

	auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo
	{
		.setLayoutCount = 0,
		.pushConstantRangeCount = 0
	};

	auto result = device.createPipelineLayout(&pipeline_layout_ci, nullptr, &pipeline_layout);

	auto gfx_pipeline_layout_ci = vk::GraphicsPipelineCreateInfo
	{
		.stageCount = 2,
		.pStages = shader_stages.data(),
		.pVertexInputState = &vert_input_ci,
		.pInputAssemblyState = &inpt_asmbly_ci,
		.pViewportState = &viewport_ci,
		.pRasterizationState = &rasterizer_ci,
		.pMultisampleState = &multisample_ci,
		.pColorBlendState = &color_blend_ci,
		.pDynamicState = &dynamic_state,
		.layout = pipeline_layout,
		.renderPass = render_pass,
		.subpass = 0
	};

	std::tie(result, graphics_pipeline) = device.createGraphicsPipeline(nullptr, gfx_pipeline_layout_ci);
	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Unable to create graphics pipeline");
	}

	device.destroyShaderModule(frag_shader);
	device.destroyShaderModule(vert_shader);
}

void renderer::create_frame_buffers()
{
	swap_chain_frame_buffers.resize(swap_chain_image_views.size());
	for(auto&& [swap_chain_frame_buffer, swap_chain_image_view] : ranges::views::zip(swap_chain_frame_buffers, swap_chain_image_views))
	{
		auto attachments = std::vector { swap_chain_image_view };
		auto frame_buffer_ci = vk::FramebufferCreateInfo
		{
			.renderPass = render_pass,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.width = swap_chain_extent.width,
			.height = swap_chain_extent.height,
			.layers = 1
		};

		swap_chain_frame_buffer = device.createFramebuffer(frame_buffer_ci);
	}
}

void renderer::create_command_pool()
{
	auto queue_family_indices = find_queue_family(physical_device, surface);

	auto command_pool_ci = vk::CommandPoolCreateInfo
	{
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = queue_family_indices.graphics_family.value()
	};

	command_pool = device.createCommandPool(command_pool_ci);
}

void renderer::create_command_buffer()
{
	auto cmd_buffer_alloc_info = vk::CommandBufferAllocateInfo
	{
		.commandPool = command_pool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	};

	command_buffers = device.allocateCommandBuffers(cmd_buffer_alloc_info);
}

void renderer::record_command_buffer(vk::CommandBuffer &cmd_buffer, uint32_t image_index)
{
	auto cmd_buff_begin_info = vk::CommandBufferBeginInfo{};
	auto result = cmd_buffer.begin(&cmd_buff_begin_info);
	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to being recordign command buffer.");
	}

	auto clear_color = vk::ClearValue
	{
		.color = std::array{0.0f, 0.0f, 0.0f, 1.0f}
	};

	auto render_pass_begin_info = vk::RenderPassBeginInfo
	{
		.renderPass = render_pass,
		.framebuffer = swap_chain_frame_buffers.at(image_index),
		.renderArea = {
			.offset = {0, 0},
			.extent = swap_chain_extent
		},
		.clearValueCount = 1,
		.pClearValues = &clear_color
	};

	cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	{
		cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline);

		auto viewport = vk::Viewport
		{
			.x = 0.0f, 
			.y = 0.0f,
			.width = static_cast<float>(swap_chain_extent.width),
			.height = static_cast<float>(swap_chain_extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		cmd_buffer.setViewport(0, viewport);

		auto scissor = vk::Rect2D
		{
			.offset = {0, 0},
			.extent = swap_chain_extent
		};
		cmd_buffer.setScissor(0, scissor);

		cmd_buffer.draw(1, 1, 0, 0);
	}
	cmd_buffer.endRenderPass();

	cmd_buffer.end();
}