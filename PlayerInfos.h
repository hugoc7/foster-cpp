#pragma once

#include <string>

struct PlayerInfos {
	std::string name;
	Uint32 id;// network ID, TODO: should be between 0 and MAX_CLIENTS
	PlayerInfos(std::string const& name, Uint32 id) :
		name{ name }, id{ id } {};
	PlayerInfos(std::string&& name, Uint32 id) :
		name{ std::move(name) }, id{ id } {};
};
