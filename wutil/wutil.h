#ifndef WUTIL_INCLUDED
#define WUTIL_INCLUDED

#include <optional>

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#define OS_WIN
#else
#error "No Windows OS macro defined"
#endif

#ifdef OS_WIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <windef.h>
#include <ShellScalingApi.h>

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

#endif

#include <vector>
#include <string>
#include <sstream>

namespace wutil
{
	namespace detail
	{
		typedef DPI_AWARENESS_CONTEXT(WINAPI * SetThreadDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
		typedef DPI_AWARENESS_CONTEXT(WINAPI * GetThreadDpiAwarenessContextProc)();
		typedef BOOL(WINAPI * EnableNonClientDpiScalingProc)(HWND);
		typedef UINT(WINAPI * GetDpiForWindowProc)(HWND);
		typedef BOOL(WINAPI * AreDpiAwarenessContextsEqualProc)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
		typedef BOOL(WINAPI * SystemParametersInfoForDpiProc)(UINT, UINT, PVOID, UINT, UINT);

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

		static const int SYMBOLS_NOT_LOADED = 0;
		static const int SYMBOLS_LOADED_AND_FOUND = 1;
		static const int SYMBOLS_LOADED_AND_NOT_FOUND = 2;

		//=========================================================================
		// dynamically load dpi functions to support older windows versions
		//===========================================================================
		int load_shcore_symbols()
		{
			static int symbol_status = SYMBOLS_NOT_LOADED;

			if (symbol_status > SYMBOLS_NOT_LOADED)
				return symbol_status;

			HMODULE shcore = LoadLibrary(TEXT("Shcore"));
			set_process_dpi_awareness = reinterpret_cast<detail::SetProcessDpiAwarenessProc>(GetProcAddress(shcore, "SetProcessDpiAwareness"));
			get_process_dpi_awareness = reinterpret_cast<detail::GetProcessDpiAwarenessProc>(GetProcAddress(shcore, "GetProcessDpiAwareness"));
			get_dpi_for_monitor = reinterpret_cast<detail::GetDpiForMonitorProc>(GetProcAddress(shcore, "GetDpiForMonitor"));

			if (set_process_dpi_awareness)
				symbol_status = SYMBOLS_LOADED_AND_FOUND;
			else
				symbol_status = SYMBOLS_LOADED_AND_NOT_FOUND;

			return symbol_status;
		}

		int load_user32_symbols()
		{
			static int symbol_status = SYMBOLS_NOT_LOADED;

			if (symbol_status > SYMBOLS_NOT_LOADED)
				return symbol_status;

			HMODULE user32 = LoadLibrary(TEXT("User32"));
			set_thread_dpi_awareness_context = reinterpret_cast<detail::SetThreadDpiAwarenessContextProc>(GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
			get_thread_dpi_awareness_context = reinterpret_cast<detail::GetThreadDpiAwarenessContextProc>(GetProcAddress(user32, "GetThreadDpiAwarenessContext"));
			enable_nonclient_dpi_scaling = reinterpret_cast<detail::EnableNonClientDpiScalingProc>(GetProcAddress(user32, "EnableNonClientDpiScaling"));
			get_dpi_for_window = reinterpret_cast<detail::GetDpiForWindowProc>(GetProcAddress(user32, "GetDpiForWindow"));
			are_dpi_awareness_contexts_equal = reinterpret_cast<detail::AreDpiAwarenessContextsEqualProc>(GetProcAddress(user32, "AreDpiAwarenessContextsEqual"));
			system_parameters_info_for_dpi = reinterpret_cast<detail::SystemParametersInfoForDpiProc>(GetProcAddress(user32, "SystemParametersInfoForDpi"));

			if (set_thread_dpi_awareness_context)
				symbol_status = SYMBOLS_LOADED_AND_FOUND;
			else
				symbol_status = SYMBOLS_LOADED_AND_NOT_FOUND;

			return symbol_status;
		}

		//======================================================
		// win32 callback functions
		//======================================================
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
	}
	
	using tstring = std::basic_string<TCHAR>;
	using tstringstream = std::basic_stringstream<TCHAR>;

	struct WindowInfo
	{
		WINDOWPLACEMENT placement = {};
		RECT rect = {};
		LONG style = 0;
		LONG ex_style = 0;
	};

	//=============================================================================
	// return container of all display devices, first item will be primary device
	//=============================================================================
	std::vector<DISPLAY_DEVICE> get_all_display_devices()
	{
		std::vector<DISPLAY_DEVICE> ddevs;

		DISPLAY_DEVICE d;
		d.cb = sizeof(d);

		int device_num = 0;
		while (EnumDisplayDevices(NULL, device_num, &d, 0))
		{
			ddevs.push_back(d);
			++device_num;

			if (d.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
				std::swap(ddevs.back(), ddevs.front());
		}

		return ddevs;
	}

	//==========================================================================
	// return container of all monitor info, first item will be primary monitor
	//==========================================================================
	std::vector<MONITORINFOEX> get_all_monitor_info()
	{
		std::vector<MONITORINFOEX> info_vec;
		EnumDisplayMonitors(NULL, NULL, detail::monitor_info_enum_proc, reinterpret_cast<LPARAM>(&info_vec));

		return info_vec;
	}

	//=============================================================
	// return monitor info for given device name
	//=============================================================
	std::optional<MONITORINFOEX> get_monitor_info(const tstring& device_name)
	{
		std::vector<MONITORINFOEX> info_vec;
		EnumDisplayMonitors(NULL, NULL, detail::monitor_info_enum_proc, reinterpret_cast<LPARAM>(&info_vec));

		for (const auto& mi : info_vec)
		{
			if (device_name.compare(mi.szDevice) == 0)
				return mi;
		}

		return {};
	}

	//==========================================================
	// return container of all display settings for monitor,
	// first item will be current display settings
	//===========================================================
	std::vector<DEVMODE> get_monitor_display_settings(const tstring& device_name)
	{
		DEVMODE dm;
		dm.dmSize = sizeof(dm);
		dm.dmDriverExtra = 0;

		std::vector<DEVMODE> disp_vec;
		int i = 0;
		while (EnumDisplaySettingsEx(device_name.c_str(), i, &dm, 0))
		{
			disp_vec.push_back(dm);
			++i;
		}

		EnumDisplaySettingsEx(device_name.c_str(), ENUM_CURRENT_SETTINGS, &dm, 0);
		for (int i = 0; i < disp_vec.size(); ++i)
		{
			// orientation and position appear to only be set for ENUM_CURRENT_SETTINGS
			if (disp_vec[i].dmBitsPerPel == dm.dmBitsPerPel &&
				disp_vec[i].dmPelsWidth == dm.dmPelsWidth &&
				disp_vec[i].dmPelsHeight == dm.dmPelsHeight &&
				disp_vec[i].dmDisplayFlags == dm.dmDisplayFlags &&
				//disp_vec[i].dmDisplayOrientation == dm.dmDisplayOrientation && 
				//disp_vec[i].dmPosition.x == dm.dmPosition.x &&
				//disp_vec[i].dmPosition.y == dm.dmPosition.y &&
				disp_vec[i].dmDisplayFrequency == dm.dmDisplayFrequency)
			{
				std::swap(disp_vec[i], disp_vec[0]);
				break;
			}

		}

		return disp_vec;
	}

	//================================================================
	// set window as fullscreen, change to monitor dimensions
	//================================================================
	std::optional<WindowInfo> set_window_fullscreen(HWND hwnd, const MONITORINFOEX& mi)
	{
		WindowInfo saved_window_info;
		saved_window_info.style = GetWindowLong(hwnd, GWL_STYLE);
		saved_window_info.ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
		GetWindowPlacement(hwnd, &saved_window_info.placement);
		GetWindowRect(hwnd, &saved_window_info.rect);

		WINDOWPLACEMENT fullscreen_placement = saved_window_info.placement;
		fullscreen_placement.showCmd = SW_SHOWNORMAL;
		fullscreen_placement.rcNormalPosition = { mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top };

		LONG res = 0;
		res += SetWindowLong(hwnd, GWL_STYLE, saved_window_info.style & ~(WS_CAPTION | WS_THICKFRAME));
		res += SetWindowLong(hwnd, GWL_EXSTYLE, saved_window_info.ex_style & ~(WS_EX_DLGMODALFRAME |
			WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
		res += SetWindowPlacement(hwnd, &fullscreen_placement);

		if (res == 0)
			return {};
		else
			return saved_window_info;
	}

	//===================================================================
	// set window as fullscreen, change to nearest monitor dimensions
	//===================================================================
	std::optional<WindowInfo> set_window_fullscreen(HWND hwnd)
	{
		WindowInfo saved_window_info;
		saved_window_info.style = GetWindowLong(hwnd, GWL_STYLE);
		saved_window_info.ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
		GetWindowPlacement(hwnd, &saved_window_info.placement);
		GetWindowRect(hwnd, &saved_window_info.rect);

		MONITORINFOEX mi;
		mi.cbSize = sizeof(mi);
		auto hmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		if (GetMonitorInfo(hmonitor, &mi) == 0)
			return {};

		WINDOWPLACEMENT fullscreen_placement = saved_window_info.placement;
		fullscreen_placement.showCmd = SW_SHOWNORMAL;
		fullscreen_placement.rcNormalPosition = { mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top };

		LONG res = 0;
		res += SetWindowLong(hwnd, GWL_STYLE, saved_window_info.style & ~(WS_CAPTION | WS_THICKFRAME));
		res += SetWindowLong(hwnd, GWL_EXSTYLE, saved_window_info.ex_style & ~(WS_EX_DLGMODALFRAME |
			WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
		res += SetWindowPlacement(hwnd, &fullscreen_placement);

		if (res == 0)
			return {};
		else
			return saved_window_info;
	}

	//====================================================
	// set window to given settings
	//======================================================
	bool set_window_to(HWND hwnd, const WindowInfo& wi)
	{
		LONG res = 0;
		res += SetWindowLong(hwnd, GWL_STYLE, wi.style);
		res += SetWindowLong(hwnd, GWL_EXSTYLE, wi.ex_style);
		res += SetWindowPlacement(hwnd, &wi.placement);

		if (res == 0)
			return false;
		else
			return true;
	}

	//====================================================================
	// change display settings, enable fullscreen flag
	//====================================================================
	std::optional<MONITORINFOEX> changes_display_settings_fullscreen(const tstring& device_name, DEVMODE& dev_mode)
	{
		auto res = ChangeDisplaySettingsEx(device_name.c_str(), &dev_mode, NULL, CDS_FULLSCREEN, NULL);
		if (res == DISP_CHANGE_SUCCESSFUL)
			return get_monitor_info(device_name);
		else
			return {};
	}

	//=====================================================
	// reset display settings to previously saved
	//======================================================
	bool reset_display_settings_fullscreen(const tstring& device_name)
	{
		auto res = ChangeDisplaySettingsEx(device_name.c_str(), NULL, NULL, 0, NULL);
		if (res == DISP_CHANGE_SUCCESSFUL)
			return true;
		else
			return false;
	}

	//============================================
	// get dpi from window
	//===========================================
	UINT get_dpi(HWND hwnd)
	{
		if (detail::load_user32_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
		{
			return detail::get_dpi_for_window(hwnd);
		}
		else if (detail::load_shcore_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
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

	//======================================
	// get dpi from point
	//=======================================
	UINT get_dpi(const POINT& point)
	{
		if (detail::load_shcore_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
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

	//===============================================
	// enable per monitor awareness
	//================================================
	bool enable_per_monitor_dpi_aware()
	{
		if (detail::load_user32_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
		{
			auto previous_context_ = detail::set_thread_dpi_awareness_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
			auto current_context = detail::get_thread_dpi_awareness_context();

			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
				return true;
			else
				return false;

		}
		else if (detail::load_shcore_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
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

	//================================================
	// enable system awareness
	//===============================================
	bool enable_system_dpi_aware()
	{
		if (detail::load_user32_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
		{
			auto previous_context_ = detail::set_thread_dpi_awareness_context(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
			auto current_context = detail::get_thread_dpi_awareness_context();

			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
				return true;
			else
				return false;

		}
		else if (detail::load_shcore_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
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

	//===========================================
	// enable unaware
	//==========================================
	bool enable_dpi_unaware()
	{
		if (detail::load_user32_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
		{
			auto previous_context_ = detail::set_thread_dpi_awareness_context(DPI_AWARENESS_CONTEXT_UNAWARE);
			auto current_context = detail::get_thread_dpi_awareness_context();

			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_UNAWARE))
				return true;
			else
				return false;

		}
		else if (detail::load_shcore_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
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

	//===================================================
	// is dpi awareness enabled
	//===================================================
	bool is_dpi_aware()
	{
		if (detail::load_user32_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
		{
			auto current_context = detail::get_thread_dpi_awareness_context();
			if (detail::are_dpi_awareness_contexts_equal(current_context, DPI_AWARENESS_CONTEXT_UNAWARE))
				return false;
			else
				return true;

		}
		else if (detail::load_shcore_symbols() == detail::SYMBOLS_LOADED_AND_FOUND)
		{
			PROCESS_DPI_AWARENESS current_awareness;
			detail::get_process_dpi_awareness(NULL, &current_awareness);

			if (current_awareness == PROCESS_DPI_UNAWARE)
				return false;
			else
				return true;
		}
		else
		{
			return false;
		}
	}

	//==================================================
	// dpi scaling helper functions
	//==================================================
	int scale_value(int value, UINT dpi)
	{
		auto dpi_scale_ = MulDiv(dpi, 100, detail::Default_DPI);
		return MulDiv(value, dpi_scale_, 100);
	}

	RECT scale_rect(const RECT& rect, UINT dpi)
	{
		auto dpi_scale_ = MulDiv(dpi, 100, detail::Default_DPI);

		auto scaled_rect = rect;
		scaled_rect.bottom = MulDiv(scaled_rect.bottom, dpi_scale_, 100);
		scaled_rect.top = MulDiv(scaled_rect.top, dpi_scale_, 100);
		scaled_rect.left = MulDiv(scaled_rect.left, dpi_scale_, 100);
		scaled_rect.right = MulDiv(scaled_rect.right, dpi_scale_, 100);

		return scaled_rect;
	}

	POINT scale_point(const POINT& point, UINT dpi)
	{
		auto dpi_scale_ = MulDiv(dpi, 100, detail::Default_DPI);

		auto scaled_point = point;
		scaled_point.x = MulDiv(scaled_point.x, dpi_scale_, 100);
		scaled_point.y = MulDiv(scaled_point.y, dpi_scale_, 100);

		return scaled_point;
	}

	HFONT scale_font(const HFONT hfont, UINT dpi)
	{
		LOGFONT log_font;
		GetObject(hfont, sizeof(log_font), &log_font);

		log_font.lfHeight = -1 * scale_value(abs(log_font.lfHeight), dpi);
		auto scaled_font = CreateFontIndirect(&log_font);

		return scaled_font;
	}
	
}

#endif