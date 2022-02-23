#pragma once

#include <Network/Network.h>

std::string getIpAdressAsString(Uint32 ip) {
	unsigned char parts[4];
	parts[0] = ip % 256;
	ip /= 256;
	parts[1] = ip % 256;
	ip /= 256;
	parts[2] = ip % 256;
	ip /= 256;
	parts[3] = ip % 256;
	return std::to_string(parts[0]) + "." + std::to_string(parts[1]) + "." + std::to_string(parts[2]) + "." + std::to_string(parts[3]);
}