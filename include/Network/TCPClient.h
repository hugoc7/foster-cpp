#pragma once


#include <iostream>
#include <string>
#include "Network.h"
#include "Containers.h"
#include "TCPNetworkNode.h"
#include "SDLNetCpp.h"
#include "TCPMessage.h"


class TCPClient : TCPNetworkNode {
public:
	moodycamel::ReaderWriterQueue<std::unique_ptr<TCPMessage>> receivedMessages;
	moodycamel::ReaderWriterQueue<std::string> messagesToSend;
	std::atomic<bool> clientRunning{ false };

	TCPClient() : TCPNetworkNode(), receivedMessages(MAX_MESSAGES) {
	}

	~TCPClient() {
		stop();
	}

	void start(std::string const& myName, std::string const& hostname, Uint16 port, Uint16 udpPort) {
		if (clientRunning) return;

		playerName = myName;
		openConnectionToServer(hostname, port);
		clientRunning = true;
		thread = std::thread(&TCPClient::loop, this, udpPort);
	}

	void stop() {
		if (!clientRunning) return;
		clientRunning = false;//std::atomic is noexcept
		thread.join();
	}

	void sendChatMsg(std::string&& text) {
		messagesToSend.emplace(std::move(text));
	}


protected:
	TCPConnection connectionToServer;

	std::thread thread{};
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

		connectionToServer.recvBufferIndex = getAvailableRecvBufferIndex(INITIAL_TCP_BUFFER_SIZE);
	}

	void closeConnection() {
		if (isConnected)
		{
			sendDisconnectRequest();
			isConnected = false;
		}
		try_releaseRecvBuffer(connectionToServer);
		connectionToServer.close(socketSet);//close the socket*/
	}

	void receiveMsg() {
			if (receivePacket(connectionToServer)) {
				//interpret the buffer content to create a msg object
				std::unique_ptr<TCPMessage> newMessage = TCPMessage::ReadFromBuffer(getRecvBuffer(connectionToServer.recvBufferIndex), connectionToServer.packetSize);
				if (newMessage.get()->type != TcpMsgType::STILL_ALIVE)
					receivedMessages.enqueue(std::move(newMessage));
			}
	}
	

	void sendDisconnectRequest() {
		DisconnectionRequest::writeToBuffer(sendingBuffer);
		sendPacket(connectionToServer, 4);
	}

	void sendConnectionRequest(Uint16 udpPort) {
		Uint16 size = ConnectionRequest::writeToBuffer(sendingBuffer, udpPort, playerName);
		sendPacket(connectionToServer, size);
	}

	void sendChatMessage(std::string const& message) {
		Uint16 size = SendChatMsg::writeToBuffer(sendingBuffer, message);
		sendPacket(connectionToServer, size);
	}

	
	void loop(Uint16 udpPort) {
		try {
			sendConnectionRequest(udpPort);
			isConnected = true;

			std::string msgToSend{};

			while (clientRunning && isRemoteAlive(connectionToServer)) {
				//here we sleep a bit in select syscall (10ms ?) to avoid crazy CPU usage
				checkSockets(TCP_SLEEP_DURATION);
				receiveMsg();

				//TODO: here if we sleep (block thread) for 1 sec it crashes ... why ?
				if (messagesToSend.try_dequeue(msgToSend)) {
					sendChatMessage(msgToSend);
				}

				keepConnectionAlive(connectionToServer);
			}

			if (!isRemoteAlive(connectionToServer))
				receivedMessages.enqueue(std::make_unique<EndOfThread>("The remote host is not alive (timeout expired)."));
		}
		catch (std::exception const& e) {
			std::cerr << e.what() << std::endl;
			receivedMessages.emplace(std::make_unique<EndOfThread>(std::string("Exception: ") + e.what()));
		}
		try {
			closeConnection();
		}
		catch(...){
		}
		//wait until the main thread close this thread
		while (clientRunning) {
			SDL_Delay(THREAD_CLOSING_DELAY);
		}

	}

	

};