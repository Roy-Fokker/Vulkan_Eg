#include "window.hpp"
#include "renderer.hpp"

auto main() -> int
{
	std::cout << "Working Directory: ";
	std::cout << std::filesystem::current_path() << "\n";

	using namespace vulkan_eg;

	// Create Window
	auto wnd = window(L"Vulkan Example",
	                  {800, 600});
	
	auto is_close{false};
	auto is_active{false};
	wnd.set_message_callback(window::message_type::keypress,
	                         [&](uintptr_t key_code, uintptr_t extension) -> bool
	{
		if (key_code == VK_ESCAPE)
		{
			is_close = true;
		}
		return true;
	});

	wnd.set_message_callback(window::message_type::activate,
	                         [&](uintptr_t state, uintptr_t wnd_id) -> bool
	{
		is_active = false;
		if (state == WA_ACTIVE or state == WA_CLICKACTIVE)
		{
			is_active = true;
		}
		return true;
	});

	// Create Renderer
	auto rndr = renderer(wnd.handle());

	wnd.show();
	while (wnd.handle() and (not is_close))
	{
		wnd.process_messages();

		if (is_active)
		{
			rndr.draw_frame();
		}
	}
	
	return EXIT_SUCCESS;
}