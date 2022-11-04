#pragma once

namespace vulkan_eg
{
	class renderer
	{
	public:
		renderer(HWND windowHandle);
		~renderer();

		void draw_frame();

	private:
		void create_vulkan_instance(std::string_view name);
		void setup_debug_callback();
		void create_surface(HWND windowHandle);
		void pick_physical_device();
		void create_logical_device();
		void create_swap_chain();
		void create_image_views();
		void create_render_pass();
		void create_graphics_pipeline();
		void create_frame_buffers();
		void create_command_pool();
		void create_command_buffer();
		void create_sync_objects();

		void record_command_buffer(vk::CommandBuffer &cmd_buffer, uint32_t image_index);


	private:
		vk::Instance instance;
		vk::DebugUtilsMessengerEXT debug_messenger;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physical_device;
		vk::Device device;
		vk::Queue graphics_queue, present_queue;
		vk::SwapchainKHR swap_chain;
		std::vector<vk::Image> swap_chain_images;
		vk::Format swap_chain_format;
		vk::Extent2D swap_chain_extent;
		std::vector<vk::ImageView> swap_chain_image_views;
		vk::RenderPass render_pass;
		vk::PipelineLayout pipeline_layout;
		vk::Pipeline graphics_pipeline;
		std::vector<vk::Framebuffer> swap_chain_frame_buffers;
		vk::CommandPool command_pool;
		std::vector<vk::CommandBuffer> command_buffers;

		vk::Semaphore image_available_semaphore;
		vk::Semaphore render_finished_semaphore;
		vk::Fence in_flight_fence;
	};
}