#pragma once

#include "SDL_net.h"
#include <iostream>
#include <string>
#include <cassert>
#include "Containers.h"
#include "Packing.h"
#include "Network.h"
#include "TCPConnection.h"


enum class TcpMsgType : Uint16 {
	//sent by client
	CONNECTION_REQUEST = 0,
	DISCONNECTION_REQUEST = 1,
	SEND_CHAT_MESSAGE = 2,

	//sent by server
	NEW_CONNECTION = 3,
	NEW_DISCONNECTION = 4,
	NEW_CHAT_MESSAGE = 5,
	PLAYER_LIST = 6,
	GOODBYE = 7,

	//send by both
	STILL_ALIVE = 8,

	//weird, used to communicat between thread...
	END_OF_THREAD = 9,


	COUNT = 10,
	//max = 255
	EMPTY = 11
};

class TCPMessage {
public:
	TcpMsgType type;
	virtual ~TCPMessage() = default;
	TCPMessage(TcpMsgType type) : type{ type } {};

	static std::unique_ptr<TCPMessage> ReadFromBuffer(UniqueByteBuffer const& buffer, int bufferSize);
	
};

class ConnectionRequest : public TCPMessage {
public:
	Uint16 udpPort;
	std::string playerName;
	std::string clientIp;//not send on network
	Uint16 playerID;//not send on network
	ConnectionRequest(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::CONNECTION_REQUEST) {
		udpPort = Packing::ReadUint16(&buffer.get()[4]);
		playerName.assign(reinterpret_cast<const char*>(buffer.get()) + 6, bufferSize - 6);
	}
	static Uint16 writeToBuffer(UniqueByteBuffer& buffer, Uint16 udpPort, std::string const& playerName);
};
class DisconnectionRequest : public TCPMessage {
public:
	Uint16 playerID;//not send on network
	std::string playerName;//not send on network

	DisconnectionRequest(std::string&& playerName, Uint16 playerID) ://used by server when a connection is closed because of exception error
		TCPMessage(TcpMsgType::DISCONNECTION_REQUEST),
		playerID{playerID},
		playerName{playerName}
	{
	}

	DisconnectionRequest(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::DISCONNECTION_REQUEST) {
	}
	static void writeToBuffer(UniqueByteBuffer& buffer);
};
class Goodbye : public TCPMessage {
public:
	std::string message;
	Goodbye(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::GOODBYE) {
		message.assign(reinterpret_cast<const char*>(buffer.get()) + 4, bufferSize - 4);
	}
	static Uint16 writeToBuffer(UniqueByteBuffer& buffer, std::string const& reason);
};
class SendChatMsg : public TCPMessage {
public:
	std::string message;
	Uint16 playerID;//not send on network
	std::string playerName;//not send on network
	SendChatMsg(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::SEND_CHAT_MESSAGE) {
		message.assign(reinterpret_cast<const char*>(buffer.get()) + 4, bufferSize - 4);
	}
	static Uint16 writeToBuffer(UniqueByteBuffer& buffer, std::string const& message);
};

class NewConnection : public TCPMessage {
public:
	Uint16 playerID;
	Uint16 entityID;
	std::string playerName;
	NewConnection(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::NEW_CONNECTION) {
		if (bufferSize < 8)
			throw std::runtime_error("Received invalid TCP message (NEW_CONNECTION) : no player or entity ID");
		playerID = SDLNet_Read16(buffer.get() + 4);
		entityID = Packing::ReadUint16(&buffer.get()[6]);
		playerName.assign(reinterpret_cast<const char*>(buffer.get()) + 8, bufferSize - 8u);
	}
	static Uint16 writeToBuffer(UniqueByteBuffer& buffer, std::string const& playerName, Uint16 playerID, Uint16 entityID);

};


class NewDisconnection : public TCPMessage {
public:
	Uint16 playerID;
	std::string playerName;//not send on network
	NewDisconnection(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::NEW_DISCONNECTION) {
		if (bufferSize < 6)
			throw std::runtime_error("Received invalid TCP message (NEW_DISCONNECTION) : no player ID");
		playerID = SDLNet_Read16(buffer.get() + 4);
	}

	static Uint16 writeToBuffer(UniqueByteBuffer& buffer, Uint16 disconnectedPlayerID);

};
class NewChatMsg : public TCPMessage {
public:
	Uint16 playerID;
	std::string message;
	NewChatMsg(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::NEW_CHAT_MESSAGE) {
		if (bufferSize < 6)
			throw std::runtime_error("Received invalid TCP message (NEW_CHAT_MESSAGE) : no player ID");
		playerID = SDLNet_Read16(buffer.get() + 4);
		message.assign(reinterpret_cast<const char*>(buffer.get()) + 6, bufferSize - 6u);
	}
	static Uint16 writeToBuffer(UniqueByteBuffer& buffer, std::string const& message, Uint16 senderPlayerID);
};

//TODO: change this it is weird, it is thread communication not network
class EndOfThread : public TCPMessage {
public:
	std::string message;
	EndOfThread(std::string&& reason) : TCPMessage(TcpMsgType::END_OF_THREAD), message{reason} {
	}
};
class PlayerList : public TCPMessage {
public:
	Uint16 udpPort;
	std::vector<std::string> playerNames;
	std::vector<Uint16> playerIDs;
	std::vector<Uint16> entityIDs;

	PlayerList(UniqueByteBuffer const& buffer, int bufferSize) : TCPMessage(TcpMsgType::PLAYER_LIST) {
		unsigned int currentByteIndex{ 4 };

		//PACKET SCHEMA (unit = byte)
		//NEW: packetSize(2) + type(2) + udpPort(2) + Nplayers * (playerId(2) + entityID(2) + nameSize(1) + name(n) ) 

		//read udp server port
		if (bufferSize < 6)
			throw std::runtime_error("Error reading player list packet (udp server port)");
		udpPort = SDLNet_Read16(buffer.get() + currentByteIndex);
		currentByteIndex += 2;

		while (currentByteIndex < bufferSize) {
			if (currentByteIndex + 1 >= bufferSize)
				throw std::runtime_error("Error reading player list packet (playerID)");
			playerIDs.emplace_back(SDLNet_Read16(buffer.get() + currentByteIndex));

			currentByteIndex += 2;
			if (currentByteIndex + 1 >= bufferSize)
				throw std::runtime_error("Error reading player list packet (entityID)");
			entityIDs.emplace_back(Packing::ReadUint16(&buffer.get()[currentByteIndex]));

			currentByteIndex += 2;
			if (currentByteIndex >= bufferSize)
				throw std::runtime_error("Error reading player list packet (playerNameSize)");
			Uint8 nameSize{ static_cast<Uint8>(buffer.get()[currentByteIndex]) };

			currentByteIndex += 1;
			if (currentByteIndex + nameSize - 1 >= bufferSize)
				throw std::runtime_error("Error reading player list packet (playerName)");
			playerNames.emplace_back(reinterpret_cast<char*>(buffer.get() + currentByteIndex), nameSize);

			currentByteIndex += nameSize;
		}
	}

	static Uint16 writeToBuffer(UniqueByteBuffer& buffer, Uint16 udpServerPort, std::vector<ClientConnection> const& connections);
};

class StillAlive : public TCPMessage {
public:
	StillAlive() : TCPMessage(TcpMsgType::STILL_ALIVE) {}
	static Uint16 writeToBuffer(UniqueByteBuffer& buffer);
};


