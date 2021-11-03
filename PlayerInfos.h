#pragma once

#include <string>

struct PlayerInfos {
	std::string name;
	Uint32 id;
	PlayerInfos(std::string const& name, Uint32 id) :
		name{ name }, id{ id } {};
	PlayerInfos(std::string&& name, Uint32 id) :
		name{ std::move(name) }, id{ id } {};
};
