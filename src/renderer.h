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

	private:
		vk::Instance instance;
	};
}