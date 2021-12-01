#pragma once

#include <string>

struct PlayerInfos {
	std::string name;
	Uint32 id;
	Uint16 udpChannel;//TODO: this field should not be here, instead we should have a NetworkManager class which handle both UDP and TCP !

	PlayerInfos(std::string const& name, Uint32 id, Uint16 udpChannel = 0) :
		name{ name }, id{ id }, udpChannel{ udpChannel } {};
	PlayerInfos(std::string&& name, Uint32 id, Uint16 udpChannel = 0) :
		name{ std::move(name) }, id{ id }, udpChannel{ udpChannel } {};
};
