#pragma once


#include "SDL_net.h"
#include <iostream>
#include <string>
#include "Network.h"
#include "Containers.h"
#include "TCPNetworkNode.h"



class Client : TCPNetworkNode {
public:
	TCPConnection connectionToServer;
	ConcurrentCircularQueue<TCPmessage> receivedMessages;
	std::thread thread{};
	std::atomic<bool> clientRunning{ false };
	UniqueByteBuffer buffer;
	int bufferSize;
	std::string playerName;
	Uint16 playerID;//should be given by the server
	bool isConnected{ false };

	//return true if the connection was successfully opened
	bool openConnectionToServer(std::string hostname, int port = 9999) {
		IPaddress ip;

		if (SDLNet_ResolveHost(&ip, hostname.c_str(), port) == -1) {
			std::cerr << "SDLNet_ResolveHost: " << SDLNet_GetError() << std::endl;
			return false;
		}

		std::cout << "Try to connect to server ..." << std::endl;
		TCPsocket tcpSock = SDLNet_TCP_Open(&ip);
		if (!tcpSock) {
			std::cerr << "SDLNet_TCP_Open: " << SDLNet_GetError() << std::endl;
			return false;
		}
		connectionToServer.setSocket(tcpSock, socketSet);


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
		return true;
	}

	void closeConnection() {
		if (isConnected)
		{
			sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::DISCONNECTION_REQUEST, "");
			isConnected = false;
		}
		connectionToServer.close();//close the socket*/
	}

	void receiveMsg() {
		try {
			if (receivePacket(connectionToServer)) {

				//interpret the buffer content to create a msg object
				TCPmessage newMessage(connectionToServer.buffer, connectionToServer.packetSize);

				switch (newMessage.type) {
				case TcpMsgType::NEW_CONNECTION:
					receivedMessages.lockQueue();
					receivedMessages.enqueue(std::move(newMessage));
					receivedMessages.unlockQueue();
					break;
				case TcpMsgType::NEW_DISCONNECTION:
					receivedMessages.lockQueue();
					receivedMessages.enqueue(std::move(newMessage));
					receivedMessages.unlockQueue();
					break;
				case TcpMsgType::PLAYER_LIST:
					receivedMessages.lockQueue();
					receivedMessages.enqueue(std::move(newMessage));
					receivedMessages.unlockQueue();
					break;
				case TcpMsgType::NEW_CHAT_MESSAGE:
					break;
				}
			}
		}
		catch (...) {
			closeConnection();
		}
	}

	/*void sendDisconnectRequest() {
		assert(bufferSize >= 4);
		SDLNet_Write16((Uint16)4, buffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::DISCONNECTION_REQUEST, buffer.get() + 2);
		SDLNet_TCP_Send(connectionToServer.tcpSocket, buffer.get(), 4);
	}
	void sendConnectionRequest() {
		Uint16 size = playerName.size() + 4;
		assert(size < 65536 && size <= MAX_TCP_PACKET_SIZE);
		if (bufferSize < size) {
			bufferSize = size;
			buffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, buffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::CONNECTION_REQUEST, buffer.get() + 2);
		std::memcpy(buffer.get() + 4, playerName.c_str(), playerName.size());
		SDLNet_TCP_Send(connectionToServer.tcpSocket, buffer.get(), size);
	}
	void sendChatMessage(std::string const& message) {
		Uint16 size = message.size() + 4;
		assert(size < 65536 && size <= MAX_TCP_PACKET_SIZE);
		if (bufferSize < size) {
			bufferSize = size;
			buffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, buffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::SEND_CHAT_MESSAGE, buffer.get() + 2);
		std::memcpy(buffer.get() + 4, message.c_str(), message.size());
		SDLNet_TCP_Send(connectionToServer.tcpSocket, buffer.get(), size);
	}*/

	void sendTCPmsg(TCPsocket tcpSock, TcpMsgType type, std::string str) {

		if (type == TcpMsgType::DISCONNECTION_REQUEST) {
			assert(bufferSize >= 4);
			SDLNet_Write16((Uint16)4, buffer.get());
			SDLNet_Write16((Uint16)type, buffer.get() + 2);
			SDLNet_TCP_Send(tcpSock, buffer.get(), 4);
		}
		else {
			Uint16 size = str.size() + 4;
			assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
			if (bufferSize < size) {
				bufferSize = size;
				buffer.lossfulRealloc(size);
			}
			SDLNet_Write16((Uint16)size, buffer.get());
			SDLNet_Write16((Uint16)type, buffer.get() + 2);
			std::memcpy(buffer.get() + 4, str.c_str(), str.size());
			SDLNet_TCP_Send(tcpSock, buffer.get(), size);
		}
	}

	void loop() {
		std::string input = "";
		sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::CONNECTION_REQUEST, playerName);
		isConnected = true;
		input = "";
		while (input != "exit" && clientRunning) {
			//receive 
			checkSockets(1);
			receiveMsg();
			
			//send
			std::cout << "Enter message: ";
			std::getline(std::cin, input);
			if (input != "exit" && !input.empty())
				sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::SEND_CHAT_MESSAGE, input);
		}
		std::cout << "Bye !" << std::endl;
		sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::DISCONNECTION_REQUEST, "");

		std::this_thread::sleep_for(std::chrono::seconds{ 2 });
	}

	Client() : TCPNetworkNode(), receivedMessages(MAX_MESSAGES), buffer(4), bufferSize{4} {

	}

	~Client() {
	}

	void stop() {
		clientRunning = false;//std::atomic is noexcept
		thread.join();
		connectionToServer.close();
	}


	void start(std::string const& myName) {
		if (clientRunning) return;

		playerName = myName;
		openConnectionToServer("127.0.0.1", 9999);
		clientRunning = true;
		thread = std::thread(&Client::loop, this);
	}

};