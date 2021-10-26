#pragma once


#include "SDL_net.h"
#include <iostream>
#include <string>
#include "Network.h"



void sendStringToTCPsocket(TCPsocket tcpSock, std::string str) {
	assert(str.size() < 65536);
	Uint16 size = str.size();
	char* buffer = new char[size+2];
	SDLNet_Write16((Uint16)size, buffer);
	memcpy(buffer+2, str.c_str(), size);
	SDLNet_TCP_Send(tcpSock, buffer, size/2 + 2);
	
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });
	
	SDLNet_TCP_Send(tcpSock, buffer + size / 2 + 2, size-size/2);
	delete[] buffer;
}

void sendTCPmsg(TCPsocket tcpSock, TcpMsgType type, std::string str) {

	if (type == TcpMsgType::DISCONNECTION_REQUEST) {
		char* buffer = new char[4];
		SDLNet_Write16((Uint16)2, buffer);
		SDLNet_Write16((Uint16)type, buffer + 2);
		SDLNet_TCP_Send(tcpSock, buffer, 4);

		/*SDLNet_TCP_Send(tcpSock, buffer, size / 2 + 2);

		std::this_thread::sleep_for(std::chrono::seconds{ 2 });

		SDLNet_TCP_Send(tcpSock, buffer + size / 2 + 2, size - size / 2);*/
		delete[] buffer;
	}
	else {

		assert(str.size() < 65536 && str.size() <= MAX_TCP_PACKET_SIZE);
		Uint16 size = str.size()+2;
		char* buffer = new char[int(size) + 2];
		SDLNet_Write16((Uint16)size, buffer);
		SDLNet_Write16((Uint16)type, buffer + 2);
		memcpy(buffer + 4, str.c_str(), str.size());
		SDLNet_TCP_Send(tcpSock, buffer, size + 2);

		/*SDLNet_TCP_Send(tcpSock, buffer, size / 2 + 2);

		std::this_thread::sleep_for(std::chrono::seconds{ 2 });

		SDLNet_TCP_Send(tcpSock, buffer + size / 2 + 2, size - size / 2);*/
		delete[] buffer;
	}
}


class Client {
public:

	

	Client() {

		// create a listening TCP socket on port 9999 (server)
		IPaddress ip;
		TCPsocket tcpSock;

		if (SDLNet_ResolveHost(&ip, "127.0.0.1", 9999) == -1) {
			std::cout << "SDLNet_ResolveHost: " << SDLNet_GetError() << std::endl;
			exit(1);
		}

		std::cout << "Try to connect to server ..." << std::endl;
		tcpSock = SDLNet_TCP_Open(&ip);
		if (!tcpSock) {
			std::cout << "SDLNet_TCP_Open: " << SDLNet_GetError() << std::endl;
			exit(2);
		}
		
		// get the remote IP and port
        // TCPsocket new_tcpsock;
		IPaddress* remote_ip;
		remote_ip = SDLNet_TCP_GetPeerAddress(tcpSock);
		if (!remote_ip) {
			printf("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
			printf("This may be a server socket.\n");
		}
		else {
			remote_ip->host;
			
			std::cout << "Connected to " << getIpAdressAsString(remote_ip->host) << " on port " << remote_ip->port << std::endl;
			
		}

		std::string input = "";

		std::cout << "Enter your name: ";
		std::cin >> input;

		sendTCPmsg(tcpSock, TcpMsgType::CONNECTION_REQUEST, input);
		input = "";
		while (input != "exit") {
			
			std::cout << "Enter message: ";
			std::cin >> input;
			if (input != "exit")
				sendTCPmsg(tcpSock, TcpMsgType::SEND_CHAT_MESSAGE, input);

			//std::this_thread::sleep_for(std::chrono::seconds{ 2 });
			//send msg
			//sendStringToTCPsocket(tcpSock, input);
		}
		std::cout << "Bye !" << std::endl;
		sendTCPmsg(tcpSock, TcpMsgType::DISCONNECTION_REQUEST, "");
		
		std::this_thread::sleep_for(std::chrono::seconds{ 2 });
		SDLNet_TCP_Close(tcpSock);
	}


};