#pragma once

namespace vulkan_eg
{
	class renderer
	{
	public:
		renderer(HWND windowHandle);
		~renderer();

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
	};
}