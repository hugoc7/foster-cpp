#pragma once
#include "SDLNetCpp.h"
#include "UDPNetwork.h"


//just echo server for now ...
class UDPClient : public UDPNetworkNode {
protected:
	
	Uint16 outgoingPacketNumber;
	Uint16 incomingPacketNumber;
	UDPpacketObject receivedPacket;
	UDPpacketObject packetToSend;
	const int UDP_CLIENT_THREAD_DELAY{ 10 };

public:

	UDPClient() : receivedPacket(MAX_UDP_PACKET_SIZE), packetToSend(100) {
	}
	~UDPClient() {
		stop();
	}
	void start(std::string const& serverIp, Uint16 serverPort, Uint16 clientPort) {
		if (running) return;
		socket.open(clientPort);
		myPort = clientPort;
		IPaddressObject remoteAddress;
		remoteAddress.resolveHost(serverIp, serverPort);
		socket.bind(0, remoteAddress);
		running = true;
		error = false;
		thread = std::thread(&UDPClient::loop, this);
	}

	void stop() {
		if (!running) return;
		running = false;//std::atomic is noexcept
		thread.join();
	}

	/*
	Uint16 id;
	Uint8 version;
	float x;
	float y;
	Uint8 type;
	*/
	void receiveNetEntitiesVec(UDPpacketObject& packet) {
		const int netEntitySize = 1 + 4 + 4 + 4 + 4 + 1;
		const int headerSize = 2 + 2;
		if (!socket.recv(packet)) return;

		assert(packet.packet->len > 4 && packet.packet->len <= MAX_UDP_PACKET_SIZE);

		//Verify checksum for integrity (against random errors)
		if (!verifyChecksum(packet)) return;

		//Update incoming packet number
		Uint16 newIncomingPacketNumber = Packing::ReadUint16(packet.packet->data);
		if (!isGreaterModulo(newIncomingPacketNumber, incomingPacketNumber)) return;
		incomingPacketNumber = newIncomingPacketNumber;

		if ((packet.packet->len - headerSize) % netEntitySize != 0 || packet.packet->len < headerSize + netEntitySize
			|| packet.packet->len > MAX_UDP_PACKET_SIZE) return;
		//const int packetVecLen = (packet.packet->len - headerSize) / netEntitySize;
		int currByte = 2;
		NetworkEntity currEntity{};
		Uint16 id{0};
		while (currByte < packet.packet->len - 2) {
			currEntity.version = Packing::ReadUint8(&packet.packet->data[currByte]);
			currByte += 1;
			currEntity.x = Packing::ReadFloat(&packet.packet->data[currByte]);
			currByte += 4;
			currEntity.y = Packing::ReadFloat(&packet.packet->data[currByte]);
			currByte += 4;
			currEntity.vx = Packing::ReadFloat(&packet.packet->data[currByte]);
			currByte += 4;
			currEntity.vy = Packing::ReadFloat(&packet.packet->data[currByte]);
			currByte += 4;
			currEntity.version = Packing::ReadUint8(&packet.packet->data[currByte]);
			currByte += 1;

			//update entity vector
			entitiesLock.lock();
			if (id >= netEntities.size())
				netEntities.resize(id + 1);
			netEntities[id] = currEntity;
			entitiesLock.unlock();


			/*std::cout << "Recv entity ("<< id <<") : " << (int)currEntity.version << "; " << (int)currEntity.type
				<< "; " << currEntity.vx << "; " << currEntity.vy << "\n";*/
			id++;
		}

	}
	

	void loop() {
		try {
			while (running) {
				SDL_Delay(UDP_CLIENT_THREAD_DELAY);
				receiveNetEntitiesVec(receivedPacket);
			}
		}
		catch (std::exception const& e) {
			std::cerr << e.what() << std::endl;
			error = true;
		}
		
		//wait until the main thread close this thread
		while (running) {
			SDL_Delay(THREAD_CLOSING_DELAY);
		}
	}
};