#include <vulkan/vulkan.hpp>

auto main() -> int
{
	auto extension_count = vk::enumerateInstanceExtensionProperties();

	std::cout << std::format("Extensions supports: {}\n", extension_count.size());

	return EXIT_SUCCESS;
}