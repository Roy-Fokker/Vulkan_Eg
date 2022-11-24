#include "pipeline.hpp"

using namespace Vulkan_eg::vkw;

namespace
{

}

pipeline::pipeline(vk::Device &device, const pipeline_descriptor &desc) 
	: vk_device{ device }
{
	create_pipeline(desc);
}

pipeline::~pipeline()
{
	vk_device.destroyPipeline(vk_pipeline);
	vk_device.destroyPipelineLayout(vk_pipeline_layout);
}

void pipeline::create_pipeline(const pipeline_descriptor &desc)
{

}