#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <windef.h>
#include <ShellScalingApi.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>

#if !defined(DPI_AWARENESS_CONTEXT_UNAWARE)

DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

typedef enum DPI_AWARENESS {
	DPI_AWARENESS_INVALID = -1,
	DPI_AWARENESS_UNAWARE = 0,
	DPI_AWARENESS_SYSTEM_AWARE = 1,
	DPI_AWARENESS_PER_MONITOR_AWARE = 2
} DPI_AWARENESS;

#define DPI_AWARENESS_CONTEXT_UNAWARE           ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE      ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((DPI_AWARENESS_CONTEXT)-3)

#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED  0x02E0
#endif

#ifndef SPI_GETICONTITLELOGFONT
#define SPI_GETICONTITLELOGFONT 0x001F
#endif

#ifndef SPI_GETICONMETRICS
#define SPI_GETICONMETRICS 0x002D
#endif

#ifndef SPI_GETNONCLIENTMETRICS
#define SPI_GETNONCLIENTMETRICS 0x0029
#endif

#ifndef DPI_ENUMS_DECLARED
#define DPI_ENUMS_DECLARED

typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef enum MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI = 1,
	MDT_RAW_DPI = 2,
	MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

#endif

namespace winutils
{
	using tstring = std::basic_string<TCHAR>;
	using tstringstream = std::basic_stringstream<TCHAR>;

	namespace detail
	{
		// Windows 10, version 1607
		typedef DPI_AWARENESS_CONTEXT(WINAPI * SetThreadDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
		typedef DPI_AWARENESS_CONTEXT(WINAPI * GetThreadDpiAwarenessContextProc)();
		typedef BOOL(WINAPI * EnableNonClientDpiScalingProc)(HWND);
		typedef UINT(WINAPI * GetDpiForWindowProc)(HWND);
		typedef BOOL(WINAPI * AreDpiAwarenessContextsEqualProc)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
		typedef BOOL(WINAPI * SystemParametersInfoForDpiProc)(UINT, UINT, PVOID, UINT, UINT);

		// Windows 8.1
		// process dpi awareness can only be set once for application
		typedef HRESULT(WINAPI * SetProcessDpiAwarenessProc)(PROCESS_DPI_AWARENESS);
		typedef HRESULT(WINAPI * GetProcessDpiAwarenessProc)(HANDLE, PROCESS_DPI_AWARENESS*);
		typedef HRESULT(WINAPI * GetDpiForMonitorProc)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

		// functions dependent on windows version may not be available
		static SetThreadDpiAwarenessContextProc set_thread_dpi_awareness_context;
		static GetThreadDpiAwarenessContextProc get_thread_dpi_awareness_context;
		static EnableNonClientDpiScalingProc enable_nonclient_dpi_scaling;
		static GetDpiForWindowProc get_dpi_for_window;
		static AreDpiAwarenessContextsEqualProc are_dpi_awareness_contexts_equal;
		static SystemParametersInfoForDpiProc system_parameters_info_for_dpi;

		static SetProcessDpiAwarenessProc set_process_dpi_awareness;
		static GetProcessDpiAwarenessProc get_process_dpi_awareness;
		static GetDpiForMonitorProc get_dpi_for_monitor;

		static const int Default_DPI = 96;
	}

	//=================================================================
	// dpi utilities
	//=================================================================

	void init()
	{
		if (!detail::set_thread_dpi_awareness_context)
		{
			HMODULE user32 = LoadLibrary(TEXT("User32"));
			detail::set_thread_dpi_awareness_context = reinterpret_cast<detail::SetThreadDpiAwarenessContextProc>(GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
			detail::get_thread_dpi_awareness_context = reinterpret_cast<detail::GetThreadDpiAwarenessContextProc>(GetProcAddress(user32, "GetThreadDpiAwarenessContext"));
			detail::enable_nonclient_dpi_scaling = reinterpret_cast<detail::EnableNonClientDpiScalingProc>(GetProcAddress(user32, "EnableNonClientDpiScaling"));
			detail::get_dpi_for_window = reinterpret_cast<detail::GetDpiForWindowProc>(GetProcAddress(user32, "GetDpiForWindow"));
			detail::are_dpi_awareness_contexts_equal = reinterpret_cast<detail::AreDpiAwarenessContextsEqualProc>(GetProcAddress(user32, "AreDpiAwarenessContextsEqual"));
			detail::system_parameters_info_for_dpi = reinterpret_cast<detail::SystemParametersInfoForDpiProc>(GetProcAddress(user32, "SystemParametersInfoForDpi"));
		}

		if (!detail::set_process_dpi_awareness)
		{
			HMODULE shcore = LoadLibrary(TEXT("Shcore"));
			detail::set_process_dpi_awareness = reinterpret_cast<detail::SetProcessDpiAwarenessProc>(GetProcAddress(shcore, "SetProcessDpiAwareness"));
			detail::get_process_dpi_awareness = reinterpret_cast<detail::GetProcessDpiAwarenessProc>(GetProcAddress(shcore, "GetProcessDpiAwareness"));
			detail::get_dpi_for_monitor = reinterpret_cast<detail::GetDpiForMonitorProc>(GetProcAddress(shcore, "GetDpiForMonitor"));
		}
	}

	UINT get_dpi(HWND hwnd)
	{
		if (detail::get_dpi_for_window)
		{
			return detail::get_dpi_for_window(hwnd);
		}
		else if (detail::get_dpi_for_monitor)
		{
			auto hmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
			UINT dpi_x = 0, dpi_y = 0;
			auto hr = detail::get_dpi_for_monitor(hmonitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
			if (hr == S_OK)
				return dpi_x;
			else
				return detail::Default_DPI;
		}
		else
			return detail::Default_DPI;
	}

	UINT get_dpi(const POINT& point)
	{
		if (detail::get_dpi_for_monitor)
		{
			auto hmonitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
			UINT dpi_x = 0, dpi_y = 0;
			auto hr = detail::get_dpi_for_monitor(hmonitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
			if (hr == S_OK)
				return dpi_x;
			else
				return detail::Default_DPI;
		}
		else
			return detail::Default_DPI;
	}

	bool enable_per_monitor_dpi_aware()
	{
		if (detail::set_thread_dpi_awareness_context)
		{
			auto previous_context_ = detail::set_thread_dpi_awareness_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
			auto current_context = detail::get_thread_dpi_awareness_context();

			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
				return true;
			else
				return false;

		}
		else if (detail::set_process_dpi_awareness)
		{
			auto result = detail::set_process_dpi_awareness(PROCESS_PER_MONITOR_DPI_AWARE);

			PROCESS_DPI_AWARENESS current_awareness;
			detail::get_process_dpi_awareness(NULL, &current_awareness);

			if (current_awareness == PROCESS_PER_MONITOR_DPI_AWARE)
				return true;
			else
				return false;
		}
		else
		{
			return false;
		}
	}

	bool enable_system_dpi_aware()
	{
		if (detail::set_thread_dpi_awareness_context)
		{
			auto previous_context_ = detail::set_thread_dpi_awareness_context(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
			auto current_context = detail::get_thread_dpi_awareness_context();

			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
				return true;
			else
				return false;

		}
		else if (detail::set_process_dpi_awareness)
		{
			auto result = detail::set_process_dpi_awareness(PROCESS_SYSTEM_DPI_AWARE);

			PROCESS_DPI_AWARENESS current_awareness;
			detail::get_process_dpi_awareness(NULL, &current_awareness);

			if (current_awareness == PROCESS_SYSTEM_DPI_AWARE)
				return true;
			else
				return false;
		}
		else
		{
			return false;
		}
	}

	bool enable_dpi_unaware()
	{
		if (detail::set_thread_dpi_awareness_context)
		{
			auto previous_context_ = detail::set_thread_dpi_awareness_context(DPI_AWARENESS_CONTEXT_UNAWARE);
			auto current_context = detail::get_thread_dpi_awareness_context();

			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_UNAWARE))
				return true;
			else
				return false;

		}
		else if (detail::set_process_dpi_awareness)
		{
			auto result = detail::set_process_dpi_awareness(PROCESS_DPI_UNAWARE);

			PROCESS_DPI_AWARENESS current_awareness;
			detail::get_process_dpi_awareness(NULL, &current_awareness);

			if (current_awareness == PROCESS_DPI_UNAWARE)
				return true;
			else
				return false;
		}
		else
		{
			return false;
		}
	}

	bool is_per_monitor_dpi_aware()
	{
		if (detail::set_thread_dpi_awareness_context)
		{
			auto current_context = detail::get_thread_dpi_awareness_context();
			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
				return true;
			else
				return false;

		}
		else if (detail::set_process_dpi_awareness)
		{
			PROCESS_DPI_AWARENESS current_awareness;
			detail::get_process_dpi_awareness(NULL, &current_awareness);

			if (current_awareness == PROCESS_PER_MONITOR_DPI_AWARE)
				return true;
			else
				return false;
		}
		else
		{
			return false;
		}
	}

	bool is_system_dpi_aware()
	{
		if (detail::set_thread_dpi_awareness_context)
		{
			auto current_context = detail::get_thread_dpi_awareness_context();
			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
				return true;
			else
				return false;

		}
		else if (detail::set_process_dpi_awareness)
		{
			PROCESS_DPI_AWARENESS current_awareness;
			detail::get_process_dpi_awareness(NULL, &current_awareness);

			if (current_awareness == PROCESS_SYSTEM_DPI_AWARE)
				return true;
			else
				return false;
		}
		else
		{
			return false;
		}
	}

	bool is_dpi_unaware()
	{
		if (detail::set_thread_dpi_awareness_context)
		{
			auto current_context = detail::get_thread_dpi_awareness_context();
			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_UNAWARE))
				return true;
			else
				return false;

		}
		else if (detail::set_process_dpi_awareness)
		{
			PROCESS_DPI_AWARENESS current_awareness;
			detail::get_process_dpi_awareness(NULL, &current_awareness);

			if (current_awareness == PROCESS_DPI_UNAWARE)
				return true;
			else
				return false;
		}
		else
		{
			return false;
		}
	}

	int dpi_scale_value(int value, UINT dpi)
	{
		auto dpi_scale_ = MulDiv(dpi, 100, detail::Default_DPI);
		return MulDiv(value, dpi_scale_, 100);
	}

	RECT dpi_scale_rect(const RECT& rect, UINT dpi)
	{
		auto dpi_scale_ = MulDiv(dpi, 100, detail::Default_DPI);

		auto scaled_rect = rect;
		scaled_rect.bottom = MulDiv(scaled_rect.bottom, dpi_scale_, 100);
		scaled_rect.top = MulDiv(scaled_rect.top, dpi_scale_, 100);
		scaled_rect.left = MulDiv(scaled_rect.left, dpi_scale_, 100);
		scaled_rect.right = MulDiv(scaled_rect.right, dpi_scale_, 100);

		return scaled_rect;
	}

	POINT dpi_scale_point(const POINT& point, UINT dpi)
	{
		auto dpi_scale_ = MulDiv(dpi, 100, detail::Default_DPI);

		auto scaled_point = point;
		scaled_point.x = MulDiv(scaled_point.x, dpi_scale_, 100);
		scaled_point.y = MulDiv(scaled_point.y, dpi_scale_, 100);

		return scaled_point;
	}

	HFONT dpi_scale_font(const HFONT hfont, UINT dpi)
	{
		LOGFONT log_font;
		GetObject(hfont, sizeof(log_font), &log_font);

		log_font.lfHeight = -1 * dpi_scale_value(abs(log_font.lfHeight), dpi);
		auto scaled_font = CreateFontIndirect(&log_font);

		return scaled_font;
	}

	//========================================================================
	// monitor and display utilities
	//========================================================================
	struct MRect
	{
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
	};

	struct Disp
	{
		TCHAR device_name[CCHDEVICENAME];
		DWORD bits_per_pixel = 0;
		DWORD pixel_width = 0;
		DWORD pixel_height = 0;
		DWORD display_flags = 0;
		DWORD display_frequency = 0;
		DWORD display_orientation = 0;
		POINTL display_position = { 0, 0 };
	};

	struct WindowInfo
	{
		WINDOWPLACEMENT placement = {};
		LONG style = NULL;
	};

	namespace detail
	{
		BOOL CALLBACK monitor_info_enum_proc(HMONITOR hmonitor, HDC hdc_monitor, LPRECT lprc_monitor, LPARAM dw_data)
		{
			auto monitor_vec = reinterpret_cast<std::vector<MONITORINFOEX>*>(dw_data);
			if (monitor_vec == nullptr)
				return false;

			MONITORINFOEX mi{};
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hmonitor, &mi);
			monitor_vec->push_back(mi);

			if (mi.dwFlags & MONITORINFOF_PRIMARY)
				std::swap(monitor_vec->back(), monitor_vec->front());

			return true;
		}

		BOOL CALLBACK monitor_bounds_enum_proc(HMONITOR hmonitor, HDC hdc_monitor, LPRECT lprc_monitor, LPARAM dw_data)
		{
			auto monitor_vec = reinterpret_cast<std::vector<MRect>*>(dw_data);
			if (monitor_vec == nullptr)
				return false;

			MONITORINFOEX mi{};
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hmonitor, &mi);
			monitor_vec->push_back(
				MRect{
				mi.rcMonitor.left,
				mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top });

			if (mi.dwFlags & MONITORINFOF_PRIMARY)
				std::swap(monitor_vec->back(), monitor_vec->front());

			return true;
		}
			   
	}

	MONITORINFOEX get_primary_monitor_info()
	{
		std::vector<MONITORINFOEX> info_vec;
		EnumDisplayMonitors(NULL, NULL, detail::monitor_info_enum_proc, reinterpret_cast<LPARAM>(&info_vec));

		if (!info_vec.empty())
			return info_vec.front();
		else
			return MONITORINFOEX{};
	}

	std::vector<MONITORINFOEX> get_all_monitors_info()
	{
		std::vector<MONITORINFOEX> info_vec;
		EnumDisplayMonitors(NULL, NULL, detail::monitor_info_enum_proc, reinterpret_cast<LPARAM>(&info_vec));

		return info_vec;
	}

	MRect get_primary_monitor_bounds()
	{
		std::vector<MRect> info_vec;
		EnumDisplayMonitors(NULL, NULL, detail::monitor_bounds_enum_proc, reinterpret_cast<LPARAM>(&info_vec));

		if (!info_vec.empty())
			return info_vec.front();
		else
			return MRect{};
	}

	std::vector<MRect> get_all_monitor_bounds()
	{
		std::vector<MRect> info_vec;
		EnumDisplayMonitors(NULL, NULL, detail::monitor_bounds_enum_proc, reinterpret_cast<LPARAM>(&info_vec));

		return info_vec;
	}

	std::vector<Disp> get_monitor_display_settings(const MONITORINFOEX& monitor_info)
	{
		DEVMODE dm;
		dm.dmSize = sizeof(dm);

		std::vector<Disp> disp_vec;
		int i = 0;
		while (EnumDisplaySettings(monitor_info.szDevice, i, &dm))
		{
			Disp d;
			std::copy(std::begin(monitor_info.szDevice), std::end(monitor_info.szDevice), std::begin(d.device_name));
			d.bits_per_pixel = dm.dmBitsPerPel;
			d.display_flags = dm.dmDisplayFlags;
			d.display_frequency = dm.dmDisplayFrequency;
			d.display_orientation = dm.dmDisplayOrientation;
			d.display_position = dm.dmPosition;
			d.pixel_height = dm.dmPelsHeight;
			d.pixel_width = dm.dmPelsWidth;

			disp_vec.push_back(d);
			++i;
		}
		
		return disp_vec;
	}

	std::vector<Disp> get_display_settings_raw(const MONITORINFOEX& monitor_info)
	{
		DEVMODE dm;
		dm.dmSize = sizeof(dm);

		std::vector<Disp> disp_vec;
		int i = 0;
		while (EnumDisplaySettingsEx(monitor_info.szDevice, i, &dm, EDS_RAWMODE))
		{
			Disp d;
			std::copy(std::begin(dm.dmDeviceName), std::end(dm.dmDeviceName), std::begin(d.device_name));
			d.bits_per_pixel = dm.dmBitsPerPel;
			d.display_flags = dm.dmDisplayFlags;
			d.display_frequency = dm.dmDisplayFrequency;
			d.display_orientation = dm.dmDisplayOrientation;
			d.display_position = dm.dmPosition;
			d.pixel_height = dm.dmPelsHeight;
			d.pixel_width = dm.dmPelsWidth;

			disp_vec.push_back(d);
			++i;
		}

		return disp_vec;
	}

	std::vector<Disp> get_display_settings_rotated(const MONITORINFOEX& monitor_info)
	{
		DEVMODE dm;
		dm.dmSize = sizeof(dm);

		std::vector<Disp> disp_vec;
		int i = 0;
		while (EnumDisplaySettingsEx(monitor_info.szDevice, i, &dm, EDS_ROTATEDMODE))
		{
			Disp d;
			std::copy(std::begin(dm.dmDeviceName), std::end(dm.dmDeviceName), std::begin(d.device_name));
			d.bits_per_pixel = dm.dmBitsPerPel;
			d.display_flags = dm.dmDisplayFlags;
			d.display_frequency = dm.dmDisplayFrequency;
			d.display_orientation = dm.dmDisplayOrientation;
			d.display_position = dm.dmPosition;
			d.pixel_height = dm.dmPelsHeight;
			d.pixel_width = dm.dmPelsWidth;

			disp_vec.push_back(d);
			++i;
		}

		return disp_vec;
	}

	Disp get_monitor_current_display_settings(const MONITORINFOEX& monitor_info)
	{
		DEVMODE dm;
		dm.dmSize = sizeof(dm);
		EnumDisplaySettingsEx(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &dm, NULL);

		Disp d;
		std::copy(std::begin(dm.dmDeviceName), std::end(dm.dmDeviceName), std::begin(d.device_name));
		d.bits_per_pixel = dm.dmBitsPerPel;
		d.display_flags = dm.dmDisplayFlags;
		d.display_frequency = dm.dmDisplayFrequency;
		d.display_orientation = dm.dmDisplayOrientation;
		d.display_position = dm.dmPosition;
		d.pixel_height = dm.dmPelsHeight;
		d.pixel_width = dm.dmPelsWidth;

		return d;
	}

	Disp get_primary_monitor_current_display_settings()
	{
		auto primary_monitor = get_primary_monitor_info();
		return get_monitor_current_display_settings(primary_monitor);
	}

	int find_matching_display_mode(DWORD bits_per_pixel, DWORD pixel_width, DWORD pixel_height, DWORD frequency, const std::vector<Disp>& dvec)
	{
		for (int i = 0; i < dvec.size(); ++i)
		{
			if (dvec[i].bits_per_pixel == bits_per_pixel &&
				dvec[i].pixel_width == pixel_width &&
				dvec[i].pixel_height == pixel_height &&
				dvec[i].display_frequency == frequency)
			{
				return i;
			}
		}

		return -1;
	}

	LONG change_display_settings(const Disp& disp, DWORD flags, LPVOID lparam)
	{
		DEVMODE dm;
		ZeroMemory(&dm, sizeof(dm));

		dm.dmSize = sizeof(dm);
		dm.dmBitsPerPel = disp.bits_per_pixel;
		dm.dmPelsHeight = disp.pixel_height;
		dm.dmPelsWidth = disp.pixel_width;
		dm.dmDisplayFrequency = disp.display_frequency;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

		return ChangeDisplaySettingsEx(disp.device_name, &dm, NULL, flags, lparam);
	}

	bool set_window_fullscreen(HWND hwnd, const Disp& disp, WindowInfo& out_info)
	{
		out_info.style = GetWindowLong(hwnd, GWL_STYLE);
		GetWindowPlacement(hwnd, &out_info.placement);

		LONG newstyle = out_info.style &= ~WS_CAPTION;

		WINDOWPLACEMENT newplacement = out_info.placement;
		newplacement.showCmd = SW_SHOWNORMAL;
		newplacement.rcNormalPosition = { 0, 0, static_cast<LONG>(disp.pixel_width), static_cast<LONG>(disp.pixel_height) };

		auto res = change_display_settings(disp, CDS_FULLSCREEN, NULL);
		if (res == DISP_CHANGE_SUCCESSFUL)
		{
			SetWindowLong(hwnd, GWL_STYLE, newstyle);
			SetWindowPlacement(hwnd, &newplacement);
			InvalidateRect(hwnd, NULL, true);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool restore_window_settings(HWND hwnd, const WindowInfo& info)
	{
		auto res = ChangeDisplaySettingsEx(NULL, 0, NULL, 0, NULL);
		if (res == DISP_CHANGE_SUCCESSFUL)
		{
			SetWindowLong(hwnd, GWL_STYLE, info.style);
			SetWindowPlacement(hwnd, &info.placement);
			InvalidateRect(hwnd, NULL, true);
			return true;
		}
		else
		{
			return false;
		}
	}
}