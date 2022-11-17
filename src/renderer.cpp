#include "renderer.hpp"

#include "vk/instance.hpp"
#include "vk/devices.hpp"
#include "vk/swap_chain.hpp"

using namespace vulkan_eg;
using namespace std::string_literals;

namespace 
{
	constexpr auto max_frames_in_flight = 2;

	auto get_window_name(HWND handle) -> std::string
	{
		auto len = static_cast<size_t>(GetWindowTextLengthA(handle)) + 1;
		auto name{""s};
		name.resize(len);
		auto result = GetWindowTextA(handle, &name[0], static_cast<int>(len));
		name.resize(len - 1);

		return name;
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

renderer::renderer(HWND windowHandle)
{
	auto name = get_window_name(windowHandle);

	vk_instance = std::make_unique<vkw::instance>(name, name, VK_MAKE_VERSION(0, 0, 1), windowHandle);
	vk_devices = std::make_unique<vkw::devices>(vk_instance.get());
	vk_swapchain = std::make_unique<vkw::swap_chain>(vk_instance.get(), vk_devices.get());

	std::tie(instance, surface) = vk_instance->get();
	device = vk_devices->get_device();

	create_graphics_pipeline();

	create_command_pool();
	create_command_buffer();
	create_sync_objects();
}

renderer::~renderer()
{
	device.waitIdle();

	for(auto&& [image_available_semaphore, render_finished_semaphore, in_flight_fence]
	         : ranges::views::zip(image_available_semaphores, render_finished_semaphores, in_flight_fences))
	{
		device.destroyFence(in_flight_fence);
		device.destroySemaphore(render_finished_semaphore);
		device.destroySemaphore(image_available_semaphore);
	}

	device.destroyCommandPool(command_pool);

	device.destroyPipeline(graphics_pipeline);
	device.destroyPipelineLayout(pipeline_layout);
}

void renderer::draw_frame()
{
	auto swap_chain = vk_swapchain->get();
	auto in_flight_fence = in_flight_fences.at(current_frame);
	auto image_available_semaphore = image_available_semaphores.at(current_frame);
	auto render_finished_semaphore = render_finished_semaphores.at(current_frame);
	auto command_buffer = command_buffers.at(current_frame);

	auto res_fence = device.waitForFences(in_flight_fence, true, UINT64_MAX);
	
	auto [result, image_index] = device.acquireNextImageKHR(swap_chain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE);
	if (result == vk::Result::eErrorOutOfDateKHR
	    or result == vk::Result::eSuboptimalKHR)
	{
		//recreate_swap_chain(graphics_queue, image_available_semaphore);
		return;
	}

	device.resetFences(in_flight_fence);

	command_buffer.reset();
	record_command_buffer(command_buffer, image_index);

	auto wait_stages = vk::PipelineStageFlags{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
	auto submit_ci = vk::SubmitInfo
	{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &image_available_semaphore,
		.pWaitDstStageMask = &wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &render_finished_semaphore
	};

	graphics_queue.submit({submit_ci}, in_flight_fence);

	auto swap_chains = std::vector{ swap_chain };
	auto present_info = vk::PresentInfoKHR
	{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &render_finished_semaphore,
		.swapchainCount = static_cast<uint32_t>(swap_chains.size()),
		.pSwapchains = swap_chains.data(),
		.pImageIndices = &image_index
	};

	result = present_queue.presentKHR(present_info);

	current_frame = (current_frame + 1) % max_frames_in_flight;
}

void renderer::create_graphics_pipeline()
{
	auto vert_shader_file = read_file("shaders/simple_shader.vert.spv");
	auto frag_shader_file = read_file("shaders/simple_shader.frag.spv");

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
	auto render_pass = vk_swapchain->get_render_pass();

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

void renderer::create_command_pool()
{
	auto queue_family_indices = vk_devices->get_queue_family();

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
		.commandBufferCount = max_frames_in_flight
	};

	command_buffers = device.allocateCommandBuffers(cmd_buffer_alloc_info);
}

void renderer::record_command_buffer(vk::CommandBuffer &cmd_buffer, uint32_t image_index)
{
	auto extent = vk_swapchain->get_extent();
	auto cmd_buff_begin_info = vk::CommandBufferBeginInfo{};
	auto result = cmd_buffer.begin(&cmd_buff_begin_info);
	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to being recording command buffer.");
	}

	auto clear_color = vk::ClearValue
	{
		.color = std::array{0.0f, 0.0f, 0.0f, 1.0f}
	};

	auto render_pass_begin_info = vk::RenderPassBeginInfo
	{
		.renderPass = vk_swapchain->get_render_pass(),
		.framebuffer = vk_swapchain->frame_buffer(image_index),
		.renderArea = {
			.offset = {0, 0},
			.extent = extent
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
			.width = static_cast<float>(extent.width),
			.height = static_cast<float>(extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		cmd_buffer.setViewport(0, viewport);

		auto scissor = vk::Rect2D
		{
			.offset = {0, 0},
			.extent = extent
		};
		cmd_buffer.setScissor(0, scissor);

		cmd_buffer.draw(3, 1, 0, 0);
	}
	cmd_buffer.endRenderPass();

	cmd_buffer.end();
}

void renderer::create_sync_objects()
{
	image_available_semaphores.resize(max_frames_in_flight);
	render_finished_semaphores.resize(max_frames_in_flight);
	in_flight_fences.resize(max_frames_in_flight);

	for(auto&& [image_available_semaphore, render_finished_semaphore, in_flight_fence]
	         : ranges::views::zip(image_available_semaphores, render_finished_semaphores, in_flight_fences))
	{
		auto semaphore_ci = vk::SemaphoreCreateInfo{};
		image_available_semaphore = device.createSemaphore(semaphore_ci);
		render_finished_semaphore = device.createSemaphore(semaphore_ci);

		auto fence_ci = vk::FenceCreateInfo
		{
			.flags = vk::FenceCreateFlagBits::eSignaled
		};
		in_flight_fence = device.createFence(fence_ci);
	}
}

// void renderer::recreate_swap_chain(vk::Queue queue, vk::Semaphore semaphore)
// {
// 	device.waitIdle();

// 	reset_semaphore(queue, semaphore);

// 	destroy_swap_chain();

// 	//create_swap_chain();
// 	create_image_views();
// 	create_frame_buffers();
// }

void renderer::reset_semaphore(vk::Queue queue, vk::Semaphore semaphore)
{
	auto psw = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eBottomOfPipe);
	auto submit_ci = vk::SubmitInfo
	{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &semaphore,
		.pWaitDstStageMask = &psw
	};

	queue.submit({submit_ci}, nullptr);
}
