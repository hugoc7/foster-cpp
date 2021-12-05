#pragma once

#include <string>
#include "ECS.h"

struct PlayerInfos {
	std::string name;
	Uint32 id;// network ID, TODO: should be between 0 and MAX_CLIENTS
	EntityID entity;
	Uint8 controls{0};


	PlayerInfos(std::string const& name, Uint32 id, EntityID entity) :
		name{ name }, id{ id }, entity{entity} {};
	PlayerInfos(std::string&& name, Uint32 id, EntityID entity) :
		name{ std::move(name) }, id{ id }, entity{entity} {};
};
