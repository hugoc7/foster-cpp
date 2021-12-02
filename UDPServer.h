#pragma once
#include "SDLNetCpp.h"
#include "UDPNetwork.h"
#include <mutex>

struct UdpClientModification {
	std::string ip;
	Uint16 port;
	int playerID;
	bool add;
	UdpClientModification(int playerID, std::string const& ip, Uint16 port) : playerID{playerID}, ip { ip }, port{ port }, add{ true } {};
	UdpClientModification(int playerID) : playerID{ playerID }, add{ false }{};
	UdpClientModification() {};
};

//Max clients: 32 (SDL_NET_MAX_UDP_CHANNELS)
class UDPServer : public UDPNetworkNode {

protected:
	UDPpacketObject packetToSend{ MAX_UDP_PACKET_SIZE };
	Uint16 outgoingPacketNumber[SDLNET_MAX_UDPCHANNELS];
	Uint16 incomingPacketNumber[SDLNET_MAX_UDPCHANNELS];
	int playerIDs[SDLNET_MAX_UDPCHANNELS];
	SparsedIndicesVector channels;

	const int UDP_SERVER_THREAD_DELAY{ 10 };

	moodycamel::ReaderWriterQueue<UdpClientModification> clientModififcationsQueue;
	const int CLIENTS_UPDATE_PERIOD{ 1000 };

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
		if (channels.Size() == 0 || netEntities.empty()) return;
		entitiesLock.lock();
		//const int netEntitySize = 1 + 4 + 4 + 4 + 4+ 1;
		//const int headerSize = 2 + 2;
		int currByte = 2;
		for (Uint16 i = 0; i < netEntities.size(); i++) {
			Packing::WriteUint8(netEntities[i].version, &packet.packet->data[currByte]);
			currByte += 1;
			Packing::WriteFloat(netEntities[i].x, &packet.packet->data[currByte]);
			currByte += 4;
			Packing::WriteFloat(netEntities[i].y, &packet.packet->data[currByte]);
			currByte += 4;
			Packing::WriteFloat(netEntities[i].vx, &packet.packet->data[currByte]);
			currByte += 4;
			Packing::WriteFloat(netEntities[i].vy, &packet.packet->data[currByte]);
			currByte += 4;
			Packing::WriteUint8(netEntities[i].type, &packet.packet->data[currByte]);
			currByte += 1;
			/*std::cout << "Send entity (" << i << ") : " << (int)netEntities[i].version << " " << (int)netEntities[i].type
				<< " " << netEntities[i].x << " " << netEntities[i].y << std::endl;*/
		}
		entitiesLock.unlock();
		packet.packet->len = currByte + 2;
		for (int channel = 0; channel < channels.Size(); channel++) {
			Packing::WriteUint16(outgoingPacketNumber[channel]++, packet.packet->data);
			calculateChecksum(packet);
			socket.send(channel, packet);
		}
	}

	void addClient(int playerID, std::string const& ip, Uint16 port) {
		clientModififcationsQueue.emplace(playerID, ip, port);
	}
	void removeClient(int playerID) {
		clientModififcationsQueue.emplace(playerID);
	}
	//TODO: optimize this by using maybe try_lock ? or lock ?
	void updateClients() {
		int channel;
		bool found;
		UdpClientModification clientModification;
		while (clientModififcationsQueue.try_dequeue(clientModification)) {
			if (clientModification.add) {
				//add client
				channel = channels.Add();
				assert(channel < SDLNET_MAX_UDPCHANNELS);
				IPaddressObject address;
				address.resolveHost(clientModification.ip, clientModification.port);
				socket.bind(channel, address);
				playerIDs[channel] = clientModification.playerID;
			}
			else {
				//TODO: Player ID should be a number between 0 and MAX_PLAYERS in all the network code !!
				//remove client
				found = false;
				for (channel = 0; channel < SDLNET_MAX_UDPCHANNELS; channel++) {
					if (playerIDs[channel] == clientModification.playerID) {
						found = true;
						break;
					}
				}
				if (!found) {
					throw std::runtime_error("Player ID not found");
				}
				socket.unbind(channel);
				channels.Remove(channel);
			}
		}		
	}

	void loop() {
		try {
			//NetworkEntity entity{ 0, 2.7, 4.2, 3 };
			//netEntities.emplace_back(entity);
			UDPpacketObject receivedPacket(MAX_UDP_PACKET_SIZE);
			Uint32 lastClientsUpdate{ SDL_GetTicks() }, now;

			while (running) {
				
				sendNetEntitiesVec(packetToSend);

				if ((now = SDL_GetTicks()) - lastClientsUpdate >= CLIENTS_UPDATE_PERIOD) {
					lastClientsUpdate = now;
					updateClients();
				}

				SDL_Delay(UDP_SERVER_THREAD_DELAY);
				//std::this_thread::sleep_for(std::chrono::milliseconds(UDP_SERVER_THREAD_DELAY));
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