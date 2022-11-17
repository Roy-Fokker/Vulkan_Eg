#pragma once

namespace vulkan_eg::vkw
{
	class instance
	{
	public:
		instance(std::string_view name, std::string_view engine, uint32_t version, HWND window_handle);
		~instance();

		instance() = delete;

		auto get() const -> std::tuple<const vk::Instance &, const vk::SurfaceKHR &>;
		auto get_layers() const -> std::vector<const char *>;

	private:
		void create_instance(std::string_view name, std::string_view engine, uint32_t version);
		void setup_debug_callback();
		void create_surface(HWND window_handle);

	private:
		vk::SurfaceKHR vk_surface;
		vk::DebugUtilsMessengerEXT debug_messenger;
		vk::Instance vk_instance;
	};
}
