#pragma once

#pragma warning(push)
#pragma warning(disable : 5105)
#include <Windows.h>
#pragma warning(pop)

namespace vulkan_eg
{
	class window
	{
	public:
		struct size
		{
			uint16_t width;
			uint16_t height;
		};

		enum class style
		{
			normal,
			borderless,
			fullscreen
		};

		enum class message_type
		{
			resize,
			activate,
			keypress
		};
		static constexpr uint8_t max_message_types = 3;

		//using callback_method = auto (*) (uintptr_t, uintptr_t) -> bool;
		using callback_method = std::function<bool(uintptr_t, uintptr_t)>;

	public:
		window() = delete;
		window(std::wstring_view title, const size &window_size, const style window_style = style::normal, uint16_t window_icon = 0);
		~window();

		void set_message_callback(message_type msg, const callback_method &callback);
		void show();
		void change_style(const style window_style);
		void change_size(const size &window_size);
		void process_messages();

		HWND handle() const;

	private:
		struct window_implementation;
		struct atl_window_implementation;

		std::unique_ptr<atl_window_implementation> window_impl;
	};
}