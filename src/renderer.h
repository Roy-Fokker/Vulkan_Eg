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

	private:
		vk::Instance instance;
		vk::DebugUtilsMessengerEXT debug_messenger;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physical_device;
		vk::Device device;
		vk::Queue graphics_queue, present_queue;
		vk::SwapchainKHR swap_chain;
	};
}