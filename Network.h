#pragma once

#include "SDL_net.h"
#include <iostream>
#include <string>
#include <cassert>
#include "Containers.h"

#define MAX_MESSAGES 100
#define MAX_16_BIT_VALUE 65535
#define MAX_8_BIT_VALUE 255
#define MAX_32_BIT_VALUE 4294967295

std::string getIpAdressAsString(Uint32 ip);

#define MAX_TCP_PACKET_SIZE 1200 //it is not the MTU though I put the same value ^^

enum class TcpMsgType {
	//sent by client
	CONNECTION_REQUEST = 0,
	DISCONNECTION_REQUEST = 1,
	SEND_CHAT_MESSAGE = 2,

	//sent by server
	NEW_CONNECTION = 3,
	NEW_DISCONNECTION = 4,
	NEW_CHAT_MESSAGE = 5,
	PLAYER_LIST = 6,
	COUNT = 7,
	//max = 255
	EMPTY = 8
};

struct TCPmessage {
	TcpMsgType type;
	std::string message;//can be either a text message or a player name
	std::string playerName;
	Uint16 playerID{};//unique ID of a player related to this message (sender/receiver)

	//only for player list packet, but TODO it should be generalized to all packets
	std::vector<std::string> playerNames;
	std::vector<Uint16> playerIDs;

	TCPmessage() = default;
	TCPmessage(TCPmessage&& other) = default;

	//called by server
	TCPmessage(TcpMsgType&& type, std::string&& playerName, Uint16 playerID) :
		playerName{ std::move(playerName) },
		type{ std::move(type) },
		playerID{ playerID }
	{
	}
	//called by client
	TCPmessage(TcpMsgType&& type, std::string const& playerName) :
		playerName{ playerName },
		type{ std::move(type) }
	{
	}
	TCPmessage(UniqueByteBuffer const& buffer, int bufferSize)
	{
		if (bufferSize < 4)
			throw std::runtime_error("Received invalid TCP message: no type");
		type = static_cast<TcpMsgType>(SDLNet_Read16(buffer.get() + 2));
		unsigned int currentByteIndex{ 4 };

		switch (type) {
		case TcpMsgType::CONNECTION_REQUEST:
			playerName.assign(buffer.get() + 4, bufferSize - 4);
			break;
		case TcpMsgType::SEND_CHAT_MESSAGE:
			message.assign(buffer.get() + 4, bufferSize - 4);
			break;
		case TcpMsgType::DISCONNECTION_REQUEST:
			//nothing
			break;
		case TcpMsgType::NEW_CONNECTION:
			if (bufferSize < 6)
				throw std::runtime_error("Received invalid TCP message (NEW_CONNECTION) : no player ID");
			playerID = SDLNet_Read16(buffer.get() + 4);
			playerName.assign(buffer.get() + 6, bufferSize - 6u);
			break;
		case TcpMsgType::NEW_DISCONNECTION:
			if (bufferSize < 6)
				throw std::runtime_error("Received invalid TCP message (NEW_DISCONNECTION) : no player ID");
			playerID = SDLNet_Read16(buffer.get() + 4);
			break;
		case TcpMsgType::NEW_CHAT_MESSAGE:
			if (bufferSize < 6)
				throw std::runtime_error("Received invalid TCP message (NEW_CHAT_MESSAGE) : no player ID");
			playerID = SDLNet_Read16(buffer.get() + 4);
			message.assign(buffer.get() + 6, bufferSize - 6u);
			break;
		case TcpMsgType::PLAYER_LIST:
			//PACKET SCHEMA (unit = byte)
			//packetSize(2) + type(2) + Nplayers * ( playerId(2) + nameSize(1) + name(n) ) 
			
			while(currentByteIndex < bufferSize) {
				if (currentByteIndex + 1 >= bufferSize)
					throw std::runtime_error("Error reading player list packet (playerID)");
				playerIDs.emplace_back(SDLNet_Read16(buffer.get() + currentByteIndex));

				currentByteIndex += 2;
				if (currentByteIndex >= bufferSize)
					throw std::runtime_error("Error reading player list packet (playerNameSize)");
				Uint8 nameSize{ static_cast<Uint8>(buffer.get()[currentByteIndex]) };

				currentByteIndex += 1;
				if (currentByteIndex + nameSize - 1 >= bufferSize)
					throw std::runtime_error("Error reading player list packet (playerName)");
				playerNames.emplace_back(buffer.get() + currentByteIndex, nameSize);

				currentByteIndex += nameSize;
			}
			break;
		default:
			throw std::runtime_error("Invalid TCP message type\n");
		}
	}

	TCPmessage& operator=(const TCPmessage& other) = default; /*{
		type = other.type;
		message = other.message;
		return *this;
	}*/
};

