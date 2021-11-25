#pragma once
#include "SDLNetCpp.h"
#include "UDPNetwork.h"

//just echo server for now ...
class UDPServer {
public:
	UDPsocketObject socket;
	bool running{true};
	void loop() {
		UDPpacketObject receivedPacket(MAX_UDP_PACKET_SIZE);

		socket.open(8880);

		//client1
		IPaddressObject ipClient1;
		ipClient1.resolveHost("localhost", 8881);
		socket.bind(0, ipClient1);

		//client2
		IPaddressObject ipClient2;
		ipClient1.resolveHost("localhost", 8882);
		socket.bind(1, ipClient2);

		while (running) {
			//if we received a new message
			if (socket.recv(receivedPacket)) {
				//echo
				socket.send(receivedPacket.packet->channel, receivedPacket);
				std::string msg(reinterpret_cast<char*>(receivedPacket.packet->data), receivedPacket.packet->len);
				std::cout << "UDP server received on channel" << receivedPacket.packet->channel << ": " << msg << std::endl;
			}
			SDL_Delay(500);
		}
	}
};