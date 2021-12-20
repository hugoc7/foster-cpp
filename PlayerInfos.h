#pragma once

#include <string>
#include "ECS.h"
#include <atomic>

struct PlayerInfos {
	std::string name;
	Uint32 id;
	EntityID entity;
	Uint8 controls{0};


	PlayerInfos(std::string const& name, Uint32 id, EntityID entity) :
		name{ name }, id{ id }, entity{entity} {};
	PlayerInfos(std::string&& name, Uint32 id, EntityID entity) :
		name{ std::move(name) }, id{ id }, entity{entity} {};
};

struct ServerPlayerInfos {
	EntityID entity{};
	bool playerListNeedsToBeSent{ false };
	Uint16 clientIndex{};


	ServerPlayerInfos(Uint16 clientIndex) :
		clientIndex{ clientIndex }
	{};
};
