#pragma once

namespace vulkan_eg::vkw
{
	class instance;

	struct queue_family
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		[[nodiscard]] auto is_complete() const -> bool;
		[[nodiscard]] auto get_array() const -> std::vector<vk::DeviceQueueCreateInfo>;
	};

	class devices
	{
	public:
		explicit devices(const instance *vkw_inst);
		~devices();

		devices() = delete;

		[[nodiscard]] auto get_queue_family() const -> queue_family;
		auto get_device() -> vk::Device &;
		auto get_physical_device() -> vk::PhysicalDevice &;

	private:
		void pick_physical_device(const instance *vkw_inst);
		void create_logical_device(const instance *vkw_inst);

	private:
		vk::PhysicalDevice vk_physical_device;
		vk::Device vk_logical_device;
		vk::Queue vk_graphics_queue, vk_present_queue;
		queue_family qf;
	};
}
