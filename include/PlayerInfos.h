#pragma once

#include <string>
#include "ECS.h"
#include <atomic>
#include <Network/UDPNetwork.h>

struct PlayerInfos {
public:
	std::string name;
	Uint32 id;
	EntityID entity;//used by server
	NetEntityID netEntityId;//used by clients
	Uint8 controls{0};


	PlayerInfos(std::string const& name, Uint32 id, EntityID entity) :
		name{ name }, id{ id }, entity{entity} {};
	PlayerInfos(std::string&& name, Uint32 id, EntityID entity) :
		name{ std::move(name) }, id{ id }, entity{entity} {};
};

