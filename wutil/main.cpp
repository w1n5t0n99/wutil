#include <iostream>

#include "wutil.h"

//#ifndef TEST_MACRO
//#pragma message ("Not defined")
//#endif

int main()
{

	auto ddevs = wutil::detail::get_all_display_devices();

	for (const auto& d : ddevs)
		std::cout << d.DeviceName << " - " << d.DeviceString << "\n";

	auto dsets = wutil::detail::get_monitor_display_settings(ddevs[1].DeviceName);

	std::cout << "=======================================================================\n";


	auto mons = wutil::detail::get_all_monitor_info();

	for (const auto& m : mons)
	{
		std::cout << m.rcMonitor.left << " " << m.rcMonitor.top << " " << m.rcMonitor.right - m.rcMonitor.left << " " << m.rcMonitor.bottom - m.rcMonitor.top << "\n";

	}

	std::cout << "=======================================================================\n";
		
	for (const auto& d : dsets)
		std::cout << d.dmPelsWidth << " x " << d.dmPelsHeight << " " << d.dmDisplayFrequency << " mhz ("
			<< d.dmPosition.x << ": x " << d.dmPosition.y << ": y) " << d.dmDisplayFlags << ": flags \n";


	auto fs_mon = wutil::detail::changes_display_settings_fullscreen(ddevs[1].DeviceName, dsets[20]);

	Sleep(5000);

	wutil::detail::reset_display_settings_fullscreen(ddevs[1].DeviceName);

	std::cin.get();
	return 0;
}