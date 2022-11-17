#pragma once

namespace vulkan_eg
{
	namespace vkw
	{
		class instance;
		class devices;
		class swap_chain;
	}

	class renderer
	{
	public:
		renderer(HWND windowHandle);
		~renderer();

		void draw_frame();

	private:
		void create_graphics_pipeline();
		void create_command_pool();
		void create_command_buffer();
		void create_sync_objects();

		void record_command_buffer(vk::CommandBuffer &cmd_buffer, uint32_t image_index);

		void reset_semaphore(vk::Queue queue, vk::Semaphore semaphore);

	private:
		std::unique_ptr<vkw::instance> vk_instance;
		std::unique_ptr<vkw::devices> vk_devices;
		std::unique_ptr<vkw::swap_chain> vk_swapchain;

		vk::Instance instance;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physical_device;
		vk::Device device;

		vk::PipelineLayout pipeline_layout;
		vk::Pipeline graphics_pipeline;
		vk::CommandPool command_pool;
		std::vector<vk::CommandBuffer> command_buffers;

		std::vector<vk::Semaphore> image_available_semaphores;
		std::vector<vk::Semaphore> render_finished_semaphores;
		std::vector<vk::Fence> in_flight_fences;

		uint32_t current_frame{0};
	};
}