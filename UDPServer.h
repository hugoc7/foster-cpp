#pragma once
#include "SDLNetCpp.h"
#include "UDPNetwork.h"
#include <mutex>

//just echo server for now ...
class UDPServer : UDPNetworkNode {

protected:
	UDPpacketObject packetToSend{ MAX_UDP_PACKET_SIZE };
	Uint16 outgoingPacketNumber[SDLNET_MAX_UDPCHANNELS];
	Uint16 incomingPacketNumber[SDLNET_MAX_UDPCHANNELS];
	SparsedIndicesVector channels;
	const int UDP_SERVER_THREAD_DELAY{ 1000 };

public:

	UDPServer() : channels(SDLNET_MAX_UDPCHANNELS) {
	}
	~UDPServer() {
		stop();
	}
	void start(Uint16 port) {
		if (running) return;
		socket.open(port);
		running = true;
		error = false;
		thread = std::thread(&UDPServer::loop, this);
	}
	
	void stop() {
		if (!running) return;
		running = false;//std::atomic is noexcept
		thread.join();
	}


	//TODO:
	void receiveClientInputs() {
		/*if (!socket.recv(packet)) return false;

		assert(packet.packet->len > 4 && packet.packet->len <= MAX_UDP_PACKET_SIZE);

		//Verify checksum for integrity (against random errors)
		if (!verifyChecksum(packet)) return false;

		Uint16 incomingPacketNumber = Packing::ReadUint16(packet.packet->data);
		if (!isGreaterModulo(incomingPacketNumber, lastIncomingPacketNumber)) return false;

		lastIncomingPacketNumber = incomingPacketNumber;
		return true;*/
	}

	//send net entities vector to all channels
	void sendNetEntitiesVec(UDPpacketObject& packet ) {
		if (channels.Size() == 0) return;

		const int netEntitySize = 2 + 1 + 4 + 4 + 1;
		const int headerSize = 2 + 2;
		int currByte = 2;
		for (Uint16 i = 0; i < netEntities.size(); i++) {
			Packing::Write(i, &packet.packet->data[currByte]);
			currByte += 2;
			Packing::Write(netEntities[i].version, &packet.packet->data[currByte]);
			currByte += 1;
			Packing::Write(netEntities[i].x, &packet.packet->data[currByte]);
			currByte += 4;
			Packing::Write(netEntities[i].y, &packet.packet->data[currByte]);
			currByte += 4;
			Packing::Write(netEntities[i].type, &packet.packet->data[currByte]);
			currByte += 1;
			std::cout << "Send entity (" << i << ") : " << (int)netEntities[i].version << " " << (int)netEntities[i].type
				<< " " << netEntities[i].x << " " << netEntities[i].y << std::endl;
		}
		packet.packet->len = currByte + 2;
		for (int channel = 0; channel < channels.Size(); channel++) {
			Packing::Write(outgoingPacketNumber[channel]++, packet.packet->data);
			calculateChecksum(packet);
			socket.send(channel, packet);
		}
	}

	int addClient(std::string const& ip, Uint16 port) {
		myLock.lock();
		int channel = channels.Add();
		IPaddressObject address;
		address.resolveHost(ip, port);
		socket.bind(channel, address);
		myLock.unlock();
		return channel;
	}
	void removeClient(int channel) {
		myLock.lock();
		socket.unbind(channel);
		channels.Remove(channel);
		myLock.unlock();
	}

	void loop() {
		try {
			NetworkEntity entity{ 0, 2.7, 4.2, 3 };
			netEntities.emplace_back(entity);
			UDPpacketObject receivedPacket(MAX_UDP_PACKET_SIZE);

			while (running) {
		
				sendNetEntitiesVec(packetToSend);
				SDL_Delay(UDP_SERVER_THREAD_DELAY);
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