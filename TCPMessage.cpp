#include "TCPMessage.h"

std::unique_ptr<TCPMessage> TCPMessage::ReadFromBuffer(UniqueByteBuffer const& buffer, int bufferSize) {

	std::unique_ptr<TCPMessage> tcpMsg;
	TcpMsgType type;
	if (bufferSize < 4)
		throw std::runtime_error("Received invalid TCP message: no type");
	Uint16 typeRead = SDLNet_Read16(buffer.get() + 2);
	if (typeRead >= static_cast<Uint16>(TcpMsgType::COUNT))
		throw std::runtime_error("Invalid TCP message type\n");
	type = static_cast<TcpMsgType>(typeRead);

	switch (type) {
	case TcpMsgType::STILL_ALIVE:
		tcpMsg = std::make_unique<StillAlive>();
		break;
	case TcpMsgType::GOODBYE:
		tcpMsg = std::make_unique<Goodbye>(buffer, bufferSize);
		break;
	case TcpMsgType::CONNECTION_REQUEST:
		tcpMsg = std::make_unique<ConnectionRequest>(buffer, bufferSize);
		break;
	case TcpMsgType::SEND_CHAT_MESSAGE:
		tcpMsg = std::make_unique<SendChatMsg>(buffer, bufferSize);
		break;
	case TcpMsgType::DISCONNECTION_REQUEST:
		tcpMsg = std::make_unique<DisconnectionRequest>(buffer, bufferSize);
		break;
	case TcpMsgType::NEW_CONNECTION:
		tcpMsg = std::make_unique<NewConnection>(buffer, bufferSize);
		break;
	case TcpMsgType::NEW_DISCONNECTION:
		tcpMsg = std::make_unique<NewDisconnection>(buffer, bufferSize);
		break;
	case TcpMsgType::NEW_CHAT_MESSAGE:
		tcpMsg = std::make_unique<NewChatMsg>(buffer, bufferSize);
		break;
	case TcpMsgType::PLAYER_LIST:
		tcpMsg = std::make_unique<PlayerList>(buffer, bufferSize);
		break;
	default:
		throw std::runtime_error("Invalid TCP message type\n");
	}

	return tcpMsg;
}

Uint16 SendChatMsg::writeToBuffer(UniqueByteBuffer& buffer, std::string const& message) {
	Uint16 size = message.size() + 4;
	assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::SEND_CHAT_MESSAGE, buffer.get() + 2);
	std::memcpy(buffer.get() + 4, message.c_str(), message.size());
	return size;
}

void DisconnectionRequest::writeToBuffer(UniqueByteBuffer& buffer) {
	assert(buffer.Size() >= 4);
	SDLNet_Write16((Uint16)4, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::DISCONNECTION_REQUEST, buffer.get() + 2);
}

Uint16 ConnectionRequest::writeToBuffer(UniqueByteBuffer& buffer, Uint16 udpPort, std::string const& playerName) {
	Uint16 size = playerName.size() + 6;
	assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::CONNECTION_REQUEST, buffer.get() + 2);
	Packing::WriteUint16(udpPort, &buffer.get()[4]);
	std::memcpy(buffer.get() + 6, playerName.c_str(), playerName.size());
	return size;
}
Uint16 Goodbye::writeToBuffer(UniqueByteBuffer& buffer, std::string const& reason) {
	Uint16 size = 4 + reason.size();
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::GOODBYE, buffer.get() + 2);
	std::memcpy(buffer.get() + 4, reason.c_str(), reason.size());
	return size;
}

Uint16 NewConnection::writeToBuffer(UniqueByteBuffer& buffer, std::string const& playerName, Uint16 playerID, Uint16 entityID) {
	Uint16 size = playerName.size() + 4 + 2 + 2;
	assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::NEW_CONNECTION, buffer.get() + 2);
	SDLNet_Write16(playerID, buffer.get() + 4);

	Packing::WriteUint16(entityID, &buffer.get()[6]);

	std::memcpy(buffer.get() + 8, playerName.c_str(), playerName.size());
	return size;
}
Uint16 NewChatMsg::writeToBuffer(UniqueByteBuffer& buffer, std::string const& message, Uint16 senderPlayerID) {
	Uint16 size = message.size() + 4 + 2;
	assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::NEW_CHAT_MESSAGE, buffer.get() + 2);
	SDLNet_Write16((Uint16)senderPlayerID, buffer.get() + 4);
	std::memcpy(buffer.get() + 6, message.c_str(), message.size());
	return size;
}

Uint16 PlayerList::writeToBuffer(UniqueByteBuffer& buffer, Uint16 udpServerPort, std::vector<ClientConnection> const& connections) {
	//PACKET SCHEMA (unit = byte)
	//packetSize(2) + type(2) + udpPort(2) + Nplayers * ( playerId(2) + playerEntityId(2) + nameSize(1) + name(n) ) 

	Uint16 size{ 6 };
	unsigned int currentByteIndex{ 6 };
	for (int i = 0; i < connections.size(); i++) {
		size += 1 + 2 + 2 + connections[i].playerName.size();
	}
	assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::PLAYER_LIST, buffer.get() + 2);
	SDLNet_Write16(udpServerPort, buffer.get() + 4);

	for (int i = 0; i < connections.size(); i++) {
		SDLNet_Write16((Uint16)connections[i].playerID, buffer.get() + currentByteIndex);
		currentByteIndex += 2;

		Packing::WriteUint16(connections[i].entityID, &buffer.get()[currentByteIndex]);
		currentByteIndex += 2;

		assert(connections[i].playerName.size() <= 255);
		buffer.get()[currentByteIndex] = static_cast<Uint8>(connections[i].playerName.size());
		currentByteIndex += 1;
		std::memcpy(buffer.get() + currentByteIndex, connections[i].playerName.c_str(), connections[i].playerName.size());
		currentByteIndex += connections[i].playerName.size();
	}
	return size;
}

Uint16 NewDisconnection::writeToBuffer(UniqueByteBuffer& buffer, Uint16 disconnectedPlayerID) {
	const Uint16 size{ 6 };
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::NEW_DISCONNECTION, buffer.get() + 2);
	SDLNet_Write16(disconnectedPlayerID, buffer.get() + 4);
	return size;
}

Uint16 StillAlive::writeToBuffer(UniqueByteBuffer& buffer) {
	const Uint16 size{ 4 };
	if (buffer.Size() < size) {
		buffer.lossfulRealloc(size);
	}
	SDLNet_Write16((Uint16)size, buffer.get());
	SDLNet_Write16((Uint16)TcpMsgType::STILL_ALIVE, buffer.get() + 2);
	return size;
}
