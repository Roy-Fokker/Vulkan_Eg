#include "window.h"

#include "atl_window_implementation.inl"

using namespace vulkan_eg;


window::window(std::wstring_view title, const size &window_size, const style window_style, uint16_t window_icon)
{
	window_impl = std::make_unique<atl_window_implementation>();

	DWORD default_window_style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	      default_window_style_ex = WS_EX_OVERLAPPEDWINDOW;

	RECT window_rectangle{ 0,
	                       0,
	                       window_size.width,
	                       window_size.height };

	AdjustWindowRectEx(&window_rectangle, default_window_style, NULL, default_window_style_ex);

	window_impl->Create(nullptr,
	                    window_rectangle,
	                    title.data(),
	                    default_window_style,
	                    default_window_style_ex);

	change_style(window_style);
	
	if (window_icon)
	{
		auto icon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(window_icon));
		window_impl->SetIcon(icon);
	}

	window_impl->CenterWindow();
}

window::~window()
{}

void window::set_message_callback(message_type msg, const callback_method &callback)
{
	uint16_t message_index = static_cast<uint16_t>(msg);
	window_impl->callback_methods[message_index] = callback;
}

void window::show()
{
	window_impl->ShowWindow(SW_SHOWNORMAL);
	window_impl->SetFocus();
}

void vulkan_eg::window::change_style(const style window_style)
{
	DWORD clear_style = WS_POPUP | WS_BORDER | WS_MINIMIZEBOX | WS_OVERLAPPEDWINDOW,
	      clear_style_ex = WS_EX_OVERLAPPEDWINDOW | WS_EX_LAYERED | WS_EX_COMPOSITED;

	DWORD new_style{},
	      new_style_ex{};
	switch (window_style)
	{
		case style::normal:
			new_style = WS_OVERLAPPEDWINDOW,
			new_style_ex = WS_EX_OVERLAPPEDWINDOW;
			break;
		case style::borderless:
		case style::fullscreen:
			new_style = WS_POPUP | WS_MINIMIZEBOX;
			break;
	}

	RECT draw_area{};
	window_impl->GetClientRect(&draw_area);

	window_impl->ModifyStyle(clear_style, new_style, SWP_FRAMECHANGED);
	window_impl->ModifyStyleEx(clear_style_ex, new_style_ex, SWP_FRAMECHANGED);

	window_impl->ResizeClient(draw_area.right, draw_area.bottom);

	window_impl->CenterWindow();

	if (window_style == style::fullscreen)
	{
		MONITORINFO monitor_info{ sizeof(MONITORINFO) };

	 	GetMonitorInfo(MonitorFromWindow(window_impl->m_hWnd, MONITOR_DEFAULTTONEAREST), &monitor_info);

		window_impl->SetWindowPos(HWND_TOP,
		                          &monitor_info.rcMonitor,
		                          SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

void vulkan_eg::window::change_size(const window::size &window_size)
{
	window_impl->ResizeClient(window_size.width, window_size.height);
	window_impl->CenterWindow();
}

void window::process_messages()
{
	BOOL has_more_messages = TRUE;
	while (has_more_messages)
	{
		MSG msg{};

		// Parameter two here has to be nullptr, putting hWnd here will
		// not retrive WM_QUIT messages, as those are posted to the thread
		// and not the window
		has_more_messages = PeekMessage(&msg, nullptr, NULL, NULL, PM_REMOVE);
		if (msg.message == WM_QUIT)
		{
			return;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

HWND window::handle() const
{
	return window_impl->m_hWnd;
}

