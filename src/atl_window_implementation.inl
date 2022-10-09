#include "window.h"

#include <atlbase.h>
#include <atlwin.h>
#include <array>
#include <string_view>

using namespace std::string_view_literals;
using namespace vulkan_eg;

struct window::atl_window_implementation : public CWindowImpl<window::atl_window_implementation>
{
	static constexpr std::wstring_view CLASSNAME = L"ATLWin32Window"sv;

	atl_window_implementation() = default;

	~atl_window_implementation()
	{
		if (m_hWnd)
		{
			DestroyWindow();
		}
	}

	DECLARE_WND_CLASS_EX(CLASSNAME.data(), 
						CS_HREDRAW | CS_VREDRAW | CS_OWNDC, 
						static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

	BEGIN_MSG_MAP(atl_window)
		MESSAGE_HANDLER(WM_DESTROY, on_wnd_destroy)
		MESSAGE_HANDLER(WM_ACTIVATEAPP, on_wnd_activate)
		MESSAGE_HANDLER(WM_SIZE, on_wnd_resize)
		MESSAGE_HANDLER(WM_KEYUP, on_wnd_keypress)
	END_MSG_MAP()

	LRESULT on_wnd_destroy(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		PostQuitMessage(NULL);
		bHandled = TRUE;
		return 0;
	}

	LRESULT on_wnd_activate(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		if (invoke_callback(message_type::activate, wParam, lParam))
		{
			bHandled = TRUE;
			return 0;
		}

		return DefWindowProc(msg, wParam, lParam);
	}

	LRESULT on_wnd_resize(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		if (invoke_callback(message_type::resize, wParam, lParam))
		{
			bHandled = TRUE;
			return 0;
		}

		return DefWindowProc(msg, wParam, lParam);
	}

	LRESULT on_wnd_keypress(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		if (invoke_callback(message_type::keypress, wParam, lParam))
		{
			bHandled = TRUE;
			return 0;
		}

		return DefWindowProc(msg, wParam, lParam);
	}

	bool invoke_callback(message_type msg, WPARAM wParam, LPARAM lParam)
	{
		uint16_t idx = static_cast<uint16_t>(msg);
		auto call = callback_methods.at(idx);
		if (call)
		{
			return call(wParam, lParam);
		}

		return false;
	}

	std::array<callback_method, max_message_types> callback_methods{ nullptr };
};