#pragma once

namespace vulkan_eg::vkw
{
	class instance;
	class devices;
	struct queue_family;

	struct surface_details
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> present_modes;
	};

	auto query_surface_details(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface)
		-> surface_details;

	class swap_chain
	{
	public:
		swap_chain(const instance *vkw_inst, devices *vkw_devices);
		~swap_chain();

		swap_chain() = delete;

		[[nodiscard]] auto get() -> vk::SwapchainKHR &;
		[[nodiscard]] auto get_render_pass() -> vk::RenderPass &;
		[[nodiscard]] auto get_extent() -> vk::Extent2D;
		[[nodiscard]] auto frame_buffer(uint32_t index) -> vk::Framebuffer &;

	private:
		void create_swap_chain(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface, const queue_family &qf);
		void create_images();
		void create_renderpass();
		void create_frame_buffers();

		void destroy_images();
		void destroy_frame_buffers();

	private:
		vk::Device vk_device;
		vk::SwapchainKHR vk_swap_chain;
		vk::Format vk_sc_format;
		vk::Extent2D vk_sc_extent;
		vk::RenderPass vk_render_pass;
		std::vector<vk::Image> vk_images;
		std::vector<vk::ImageView> vk_image_views;
		std::vector<vk::Framebuffer> vk_frame_buffers;
	};
};