#pragma once
#include "SDLNetCpp.h"
#include "UDPNetwork.h"
#include <mutex>
#include "PlayerInfos.h"
#include <chrono>



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
public:

	UDPServer()  {
		for (int i = 0; i < SDLNET_MAX_UDPCHANNELS; i++) {
			outgoingPacketNumber[i] = 0;
			incomingPacketNumber[i] = 0xFFFF;
			clientInputs[i] = 0;
		}
	}
	~UDPServer() {
		stop();
	}
	void start(Uint16 port) {
		if (running) return;
		socket.open(port);
		running = true;
		error = false;
		channels.clear();
		thread = std::thread(&UDPServer::loop, this);
	}
	
	void stop() {
		if (!running) return;
		running = false;//std::atomic is noexcept
		thread.join();
	}
	void addClient(int playerID, std::string const& ip, Uint16 port) {
		clientModififcationsQueue.emplace(playerID, ip, port);
	}
	void removeClient(int playerID) {
		clientModififcationsQueue.emplace(playerID);
	}
 
	void getClientInputs(ArrayMap<PlayerInfos>& playerInfos)  {
		std::lock_guard<std::mutex> lock(clientInputsMutex);

		//assert(playerInfos.size() == channels.Size());
		for (int i = 0; i < channels.size(); i++) {
			if(playerInfos.Contains(channels[i]))
				playerInfos.Get(channels[i]).controls = clientInputs[channels[i]];
		}
	}

	void writeEntitiesToPacket(std::vector<int> const& netIDs, std::vector<int> const& entityIDs, std::vector<int> const& versions, 
		std::vector<EntityType> const& types, ECS const& ecs_arg) 
	{
		std::lock_guard<std::mutex> lockServer(entitiesMutex);
		static const int netEntitySize = 2 + 1 + 4 + 4 + 4 + 4 + 1;

		int currByte = 2;//2 bytes for packet number
		int newPacketSize = 2 + netEntitySize * netIDs.size() + 2;
		assert(newPacketSize <= MAX_UDP_PACKET_SIZE);
		if (packetToSend.packet->maxlen < newPacketSize) {
			packetToSend.resize(newPacketSize);
		}
		packetToSend.packet->len = newPacketSize;

		//for each networked entity
		for (int j = 0; j < netIDs.size(); j++) {
			int netID = netIDs[j];
			
			//TODO: build a different UDP packet for each player, based on his camera


			MovingObject const& movingComp = ecs_arg.getComponent<MovingObject>(entityIDs.at(netID));

			Packing::WriteUint16(netID, &packetToSend.packet->data[currByte]);
			currByte += 2;
			Packing::WriteUint8(versions.at(netID), &packetToSend.packet->data[currByte]);
			currByte += 1;
			Packing::WriteFloat(movingComp.position.x, &packetToSend.packet->data[currByte]);
			currByte += 4;
			Packing::WriteFloat(movingComp.position.y, &packetToSend.packet->data[currByte]);
			currByte += 4;
			Packing::WriteFloat(movingComp.newSpeed.x, &packetToSend.packet->data[currByte]);
			currByte += 4;
			Packing::WriteFloat(movingComp.newSpeed.x, &packetToSend.packet->data[currByte]);
			currByte += 4;
			Packing::WriteUint8(static_cast<Uint8>(types.at(netID)), &packetToSend.packet->data[currByte]);
			currByte += 1;
			/*std::cout << "Send entity (" << i << ") : " << (int)netEntities[i].version << " " << (int)netEntities[i].type
				<< " " << netEntities[i].x << " " << netEntities[i].y << std::endl;*/
		}
	}

protected:

	UDPpacketObject packetToSend{ MAX_UDP_PACKET_SIZE };
	Uint16 outgoingPacketNumber[SDLNET_MAX_UDPCHANNELS];
	Uint16 incomingPacketNumber[SDLNET_MAX_UDPCHANNELS];
	std::chrono::time_point<std::chrono::steady_clock> lastTimeEntitiesSent;

	const int sendEntitiesPeriod{ 30 };
	const int UDP_SERVER_THREAD_DELAY{ 5 };

	moodycamel::ReaderWriterQueue<UdpClientModification> clientModififcationsQueue;
	const int CLIENTS_UPDATE_PERIOD{ 1000 };
	Uint8 clientInputs[SDLNET_MAX_UDPCHANNELS];
	std::mutex clientInputsMutex;
	std::vector<int> channels;

	void receiveClientInputs(UDPpacketObject& packet) {
		if (!socket.recv(packet)) return;

		//TODO: throw an exception, and then handle this error
		assert(packet.packet->len > 4 && packet.packet->len <= MAX_UDP_PACKET_SIZE);

		//Verify checksum for integrity (against random errors)
		if (!verifyChecksum(packet)) {
			std::cout << "bad checksum\n";
			return;
		}

		Uint16 newPacketNumber = Packing::ReadUint16(packet.packet->data);
		if (!isGreaterModulo(newPacketNumber, incomingPacketNumber[packet.packet->channel]))
		{
			std::cout << "bad packet number\n";
			return;
		}

		incomingPacketNumber[packet.packet->channel] = newPacketNumber;

		if (packet.packet->len != 5) {
			std::cout << "bad packet len\n";
			return; //TODO: throw an error
		}

		Uint8 recvControls = Packing::ReadUint8(&packet.packet->data[2]);
		//if (recvControls != 0) {
			std::cout << packet.packet->channel << " RECV INPUT: "<< (int)packet.packet->data[2] << "\n";
		//}
		std::lock_guard<std::mutex> lock(clientInputsMutex);
		clientInputs[packet.packet->channel] = recvControls;
	}

	//send net entities vector to all channels
	void sendNetEntities(UDPpacketObject& packet ) {
		if (channels.empty()) return;

		std::lock_guard<std::mutex> lock(entitiesMutex);
		//const int netEntitySize = 1 + 4 + 4 + 4 + 4+ 1;
		//const int headerSize = 2 + 2;
		for (int i = 0; i < channels.size(); i++) {
			Packing::WriteUint16(outgoingPacketNumber[channels[i]]++, packet.packet->data);
			calculateChecksum(packet);
			socket.send(channels[i], packet);
		}
	}

	
	//TODO: optimize this by using maybe try_lock ? or lock ?
	void updateClients() {
		UdpClientModification clientModification;
		while (clientModififcationsQueue.try_dequeue(clientModification)) {

			int channel{ clientModification.playerID }; // udp schannel = player ID !
			assert(channel >= 0 && channel < SDLNET_MAX_UDPCHANNELS);//TODO: should we throw an exception ?

			if (clientModification.add) {
				IPaddressObject address;
				address.resolveHost(clientModification.ip, clientModification.port);
				socket.bind(channel, address);
				outgoingPacketNumber[channel] = 0;
				incomingPacketNumber[channel] = 0xFFFF;
				clientInputs[channel] = 0;
				channels.push_back(channel);
			}
			else {
				//remove client
				socket.unbind(channel);
				for (int i = 0; i < channels.size(); i++) {
					if (channel == channels[i]) {
						removeFromVector(channels, i);
					}
				}
			}
		}		
	}

	void loop() {
		std::chrono::milliseconds ms;
		lastTimeEntitiesSent = std::chrono::steady_clock::now();
		try {
			//NetworkEntity entity{ 0, 2.7, 4.2, 3 };
			//netEntities.emplace_back(entity);
			UDPpacketObject receivedPacket(MAX_UDP_PACKET_SIZE);
			Uint32 lastClientsUpdate{ SDL_GetTicks() }, now;

			while (running) {
				auto now = std::chrono::steady_clock::now();
				if (now - lastTimeEntitiesSent >= sendEntitiesPeriod * ms) {
					lastTimeEntitiesSent = now;
					sendNetEntities(packetToSend);
				}

				//if ((now = SDL_GetTicks()) - lastClientsUpdate >= CLIENTS_UPDATE_PERIOD) {
					//lastClientsUpdate = now;
					updateClients();
				//}

				receiveClientInputs(receivedPacket);

				//SDL_Delay(UDP_SERVER_THREAD_DELAY);
				std::this_thread::sleep_for(std::chrono::milliseconds(UDP_SERVER_THREAD_DELAY));
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