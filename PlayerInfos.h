#pragma once

#include <string>

struct PlayerInfos {
	std::string name;
	int id;
	PlayerInfos(std::string const& name, int id) :
		name{ name }, id{ id } {};
};
