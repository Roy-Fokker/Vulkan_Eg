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

	private:
		vk::Instance instance;
		vk::DebugUtilsMessengerEXT debug_messenger;
	};
}