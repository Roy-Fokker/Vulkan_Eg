#include "window.h"

auto main() -> int
{
	auto extension_count = vk::enumerateInstanceExtensionProperties();
	std::cout << std::format("Extensions supports: {}\n", extension_count.size());

	using namespace vulkan_eg;
	auto wnd = window(L"Vulkan Example",
	                  {800, 600});
	
	auto is_close{false};
	wnd.set_message_callback(window::message_type::keypress,
	                         [&](uintptr_t key_code, uintptr_t extension) -> bool
	{
		if (key_code == VK_ESCAPE)
		{
			is_close = true;
		}
		return true;
	});

	wnd.show();
	while (wnd.handle() and (not is_close))
	{
		wnd.process_messages();
	}
	
	return EXIT_SUCCESS;
}