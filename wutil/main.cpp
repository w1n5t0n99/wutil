#include <iostream>

#include "wutil.h"

int main()
{

	auto ddevs = wutil::detail::get_all_display_devices();

	for (const auto& d : ddevs)
		std::cout << d.DeviceName << " - " << d.DeviceString << "\n";

	auto dsets = wutil::detail::get_monitor_display_settings(ddevs[0].DeviceName);

	std::cout << "=======================================================================\n";
		
	for (const auto& d : dsets)
		std::cout << d.dmPelsWidth << " x " << d.dmPelsHeight << " " << d.dmDisplayFrequency << " mhz ("
			<< d.dmPosition.x << ": x " << d.dmPosition.y << ": y) " << d.dmDisplayFlags << ": flags \n";

	std::cin.get();
	return 0;
}