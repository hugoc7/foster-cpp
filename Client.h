#pragma once


#include <iostream>
#include <string>
#include "Network.h"
#include "Containers.h"
#include "TCPNetworkNode.h"
#include "SDLNetCpp.h"



class Client : TCPNetworkNode {
public:
	TCPConnection connectionToServer;
	moodycamel::ReaderWriterQueue<TCPmessage> receivedMessages;
	moodycamel::ReaderWriterQueue<TCPmessage> messagesToSend;

	std::thread thread{};
	std::atomic<bool> clientRunning{ false };
	UniqueByteBuffer buffer;
	int bufferSize;
	std::string playerName;
	Uint16 playerID;//should be given by the server
	bool isConnected{ false };

	//return true if the connection was successfully opened
	void openConnectionToServer(std::string const& hostname, int port = 9999) {
		IPaddressObject ip;
		ip.resolveHost(hostname.c_str(), port);
		std::cout << "Try to connect to server ..." << std::endl;
		TCPsocketObject tcpSock;
		tcpSock.open(ip);
		connectionToServer.setSocket(std::move(tcpSock), socketSet);

		// get the remote IP and port
		std::cout << "Connected to " << connectionToServer.tcpSocket.getPeerIP() 
			<< " on port " << connectionToServer.tcpSocket.getPeerPort() << std::endl;
	}

	void closeConnection() {
		if (isConnected)
		{
			sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::DISCONNECTION_REQUEST, "");
			isConnected = false;
		}
		connectionToServer.close(socketSet);//close the socket*/
	}

	void receiveMsg() {
			if (receivePacket(connectionToServer)) {
				//interpret the buffer content to create a msg object
				TCPmessage newMessage(connectionToServer.buffer, connectionToServer.packetSize);
				receivedMessages.enqueue(std::move(newMessage));
			}
	}
	void sendChatMsg(std::string&& text) {
		messagesToSend.emplace(TcpMsgType::SEND_CHAT_MESSAGE, std::move(text));
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

	void sendTCPmsg(TCPsocketObject const& tcpSock, TcpMsgType type, std::string str) {

		if (type == TcpMsgType::DISCONNECTION_REQUEST) {
			assert(bufferSize >= 4);
			SDLNet_Write16((Uint16)4, buffer.get());
			SDLNet_Write16((Uint16)type, buffer.get() + 2);
			tcpSock.send(buffer.get(), 4);
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
			tcpSock.send(buffer.get(), size);
		}
	}

	void loop() {

		sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::CONNECTION_REQUEST, playerName);
		isConnected = true;

		int k = 5000;
		TCPmessage msgToSend{};
		try {
			while ( clientRunning) {
				//here we sleep a bit in select syscall (10ms ?) to avoid CPU crazy usage
				checkSockets(TCP_SLEEP_DURATION);
				receiveMsg();

				//TODO: here if we sleep (block thread) for 1 sec it crashes ... why ?
				if (messagesToSend.try_dequeue(msgToSend)) {
					sendTCPmsg(connectionToServer.tcpSocket, msgToSend.type, msgToSend.message);
				}
			}
			std::cout << "Bye !" << std::endl;
			sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::DISCONNECTION_REQUEST, "");
		}
		catch (std::exception const& e) {
			std::cerr << e.what() << std::endl;
			closeConnection();
		}

		std::this_thread::sleep_for(std::chrono::seconds{ 2 });
	}

	Client() : TCPNetworkNode(), receivedMessages(MAX_MESSAGES), buffer(4), bufferSize{4} {

	}

	~Client() {
		stop();
	}

	void stop() {
		if (!clientRunning) return;
		clientRunning = false;//std::atomic is noexcept
		thread.join();
		connectionToServer.close(socketSet);
	}


	void start(std::string const& myName, std::string const& hostname = "127.0.0.1", Uint16 port = 9999) {
		if (clientRunning) return;

		playerName = myName;
		openConnectionToServer(hostname, port);
		clientRunning = true;
		thread = std::thread(&Client::loop, this);
	}

};