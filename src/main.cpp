#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

auto main() -> int
{
	auto extension_count = vk::enumerateInstanceExtensionProperties();
	std::cout << std::format("Extensions supports: {}\n", extension_count.size());

	auto matrix = glm::mat4{};
	auto vec = glm::vec4{};
	auto test = matrix * vec;

	return EXIT_SUCCESS;
}