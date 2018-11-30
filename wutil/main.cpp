#include <iostream>

#include "wutil.h"

int main()
{

	auto ddevs = wutil::detail::get_all_display_devices();

	for (const auto& d : ddevs)
		std::cout << d.DeviceName << " - " << d.DeviceString << "\n";

	auto dsets = wutil::detail::get_monitor_display_settings(ddevs[0].DeviceName);

	std::cout << "=======================================================================";
		
	for (const auto& d : dsets)
		std::cout << d.dmPelsWidth << " x " << d.dmPelsHeight << " " << d.dmBitsPerPel << " bpp\n";

	std::cin.get();
	return 0;
}