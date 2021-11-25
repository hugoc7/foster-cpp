#pragma once
#include "SDLNetCpp.h"
#include "UDPNetwork.h"

//just echo server for now ...
class UDPClient {
public:
	UDPsocketObject socket;
	bool running{ true };
	void loop(int port) {
		UDPpacketObject receivedPacket(MAX_UDP_PACKET_SIZE);
		UDPpacketObject packetToSend(100);

		std::string msgToSend = "Hello my friend !";
		std::memcpy(packetToSend.packet->data, msgToSend.c_str(), msgToSend.size());
		packetToSend.packet->len = msgToSend.size();


		socket.open(port);

		//server
		IPaddressObject ipServer;
		ipServer.resolveHost("localhost", 8880);
		socket.bind(0, ipServer);

		while (running) {
			//if we received a new message
			if (socket.recv(receivedPacket)) {
				std::string msg(reinterpret_cast<char*>(receivedPacket.packet->data), receivedPacket.packet->len);
				std::cout << "UDP client ("<< port <<") received: " << msg << std::endl;
			}
			socket.send(0, packetToSend);
			SDL_Delay(1000);
		}
	}
};