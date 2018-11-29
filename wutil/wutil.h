#ifndef WUTIL_INCLUDED
#define WUTIL_INCLUDED

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
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>

namespace wutil
{
	using tstring = std::basic_string<TCHAR>;
	using tstringstream = std::basic_stringstream<TCHAR>;

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

		void load_dpi_functions()
		{
			static bool IS_DPI_LOADED = false;

			if (IS_DPI_LOADED)
				return;

			HMODULE user32 = LoadLibrary(TEXT("User32"));
			detail::set_thread_dpi_awareness_context = reinterpret_cast<detail::SetThreadDpiAwarenessContextProc>(GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
			detail::get_thread_dpi_awareness_context = reinterpret_cast<detail::GetThreadDpiAwarenessContextProc>(GetProcAddress(user32, "GetThreadDpiAwarenessContext"));
			detail::enable_nonclient_dpi_scaling = reinterpret_cast<detail::EnableNonClientDpiScalingProc>(GetProcAddress(user32, "EnableNonClientDpiScaling"));
			detail::get_dpi_for_window = reinterpret_cast<detail::GetDpiForWindowProc>(GetProcAddress(user32, "GetDpiForWindow"));
			detail::are_dpi_awareness_contexts_equal = reinterpret_cast<detail::AreDpiAwarenessContextsEqualProc>(GetProcAddress(user32, "AreDpiAwarenessContextsEqual"));
			detail::system_parameters_info_for_dpi = reinterpret_cast<detail::SystemParametersInfoForDpiProc>(GetProcAddress(user32, "SystemParametersInfoForDpi"));
			HMODULE shcore = LoadLibrary(TEXT("Shcore"));
			detail::set_process_dpi_awareness = reinterpret_cast<detail::SetProcessDpiAwarenessProc>(GetProcAddress(shcore, "SetProcessDpiAwareness"));
			detail::get_process_dpi_awareness = reinterpret_cast<detail::GetProcessDpiAwarenessProc>(GetProcAddress(shcore, "GetProcessDpiAwareness"));
			detail::get_dpi_for_monitor = reinterpret_cast<detail::GetDpiForMonitorProc>(GetProcAddress(shcore, "GetDpiForMonitor"));

			IS_DPI_LOADED = true;
			return;
		}
	}
}

#endif