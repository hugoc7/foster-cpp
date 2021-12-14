#pragma once
#include "SDLNetCpp.h"
#include "UDPNetwork.h"
#include "UserControls.h"
#include "ReaderWriterQueue.h"
#include <atomic>

//just echo server for now ...
class UDPClient : public UDPNetworkNode {
protected:

	Uint16 outgoingPacketNumber{ 0x0000 };
	Uint16 incomingPacketNumber{ 0xFFFF };
	UDPpacketObject receivedPacket;
	UDPpacketObject packetToSend;
	const int UDP_CLIENT_THREAD_DELAY{ 5 };
	//moodycamel::ReaderWriterQueue<ActionKey> userInputs;

public:
	std::atomic<Uint8> userInputs;
	std::atomic<Uint8> lastUserInputs;

	UDPClient() : receivedPacket(MAX_UDP_PACKET_SIZE), packetToSend(100) {
	}
	~UDPClient() {
		stop();
	}

	//TODO: send all input information in each packet: current state + lastState + list of event which happened between them
	//and maybe try to use atomic (1 atomic byte for each state and a 4 bytes(uint) for event list stored in base 3 for UP, LEFT, RIGHT) ?
	void sendUserInputs() {
		Uint8 inputToSend = userInputs.load();
		if (inputToSend == lastUserInputs) return;
		lastUserInputs = inputToSend;
		packetToSend.packet->len = 5;
		Packing::WriteUint16(outgoingPacketNumber++, &packetToSend.packet->data[0]);
		Packing::WriteUint8(userInputs, &packetToSend.packet->data[2]);	
		std::cout << "SEND INPUT: "<< (int)userInputs << "\n";
	    calculateChecksum(packetToSend);
		socket.send(0, packetToSend);
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
		if (!verifyChecksum(packet)){
			std::cerr << "bad checksum\n";
			return;
		}

		//Update incoming packet number
		Uint16 newIncomingPacketNumber = Packing::ReadUint16(packet.packet->data);
		if (!isGreaterModulo(newIncomingPacketNumber, incomingPacketNumber)) {
			std::cerr << "bad packet number\n";
			return;
		}
		incomingPacketNumber = newIncomingPacketNumber;

		if ((packet.packet->len - headerSize) % netEntitySize != 0 || packet.packet->len < headerSize + netEntitySize
			|| packet.packet->len > MAX_UDP_PACKET_SIZE) {
			std::cout << "bad length\n";
			return;
		}
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
				//SDL_Delay(UDP_CLIENT_THREAD_DELAY);
				std::this_thread::sleep_for(std::chrono::milliseconds(UDP_CLIENT_THREAD_DELAY));
				receiveNetEntitiesVec(receivedPacket);
				sendUserInputs();
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