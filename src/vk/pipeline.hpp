#pragma once

namespace Vulkan_eg::vkw
{
	struct pipeline_descriptor
	{
		using file_data = std::vector<uint32_t>;

		std::vector<std::tuple<vk::ShaderStageFlags, file_data>> shaders;
		vk::PrimitiveTopology topology;
		vk::PolygonMode polygon_mode;
		vk::CullModeFlags cull_mode;
		vk::FrontFace front_face;
	};

	class pipeline
	{
	public:
		pipeline() = delete;
		pipeline(vk::Device &device, const pipeline_descriptor &desc);
		~pipeline();

	private:
		void create_pipeline(const pipeline_descriptor &desc);

	private:
		vk::Device vk_device;
		vk::PipelineLayout vk_pipeline_layout;
		vk::Pipeline vk_pipeline;
	};
}
