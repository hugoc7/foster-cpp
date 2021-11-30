#pragma once


#include <iostream>
#include <string>
#include "Network.h"
#include "Containers.h"
#include "TCPNetworkNode.h"
#include "SDLNetCpp.h"



class TCPClient : TCPNetworkNode {
public:
	moodycamel::ReaderWriterQueue<TCPmessage> receivedMessages;
	moodycamel::ReaderWriterQueue<TCPmessage> messagesToSend;
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
		messagesToSend.emplace(TcpMsgType::SEND_CHAT_MESSAGE, std::move(text));
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
				TCPmessage newMessage(getRecvBuffer(connectionToServer.recvBufferIndex), connectionToServer.packetSize);
				if (newMessage.type != TcpMsgType::STILL_ALIVE)
					receivedMessages.enqueue(std::move(newMessage));
			}
	}
	

	void sendDisconnectRequest() {
		assert(sendingBuffer.Size() >= 4);
		SDLNet_Write16((Uint16)4, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::DISCONNECTION_REQUEST, sendingBuffer.get() + 2);
		sendPacket(connectionToServer, 4);
	}

	void sendConnectionRequest(Uint16 udpPort) {

		Uint16 size = playerName.size() + 6;
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (sendingBuffer.Size() < size) {
			sendingBuffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::CONNECTION_REQUEST, sendingBuffer.get() + 2);
		Packing::Write(udpPort, &sendingBuffer.get()[4]);
		std::memcpy(sendingBuffer.get() + 6, playerName.c_str(), playerName.size());
		sendPacket(connectionToServer, size);
	}

	void sendChatMessage(std::string const& message) {
		Uint16 size = message.size() + 4;
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (sendingBuffer.Size() < size) {
			sendingBuffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::SEND_CHAT_MESSAGE, sendingBuffer.get() + 2);
		std::memcpy(sendingBuffer.get() + 4, message.c_str(), message.size());
		sendPacket(connectionToServer, size);
	}

	
	void loop(Uint16 udpPort) {
		try {
			sendConnectionRequest(udpPort);
			isConnected = true;

			TCPmessage msgToSend{};

			//TODO: the thread should be joinable so if the loop end, just send a message to the main thread and wait
			while (clientRunning && isRemoteAlive(connectionToServer)) {
				//here we sleep a bit in select syscall (10ms ?) to avoid crazy CPU usage
				checkSockets(TCP_SLEEP_DURATION);
				receiveMsg();

				//TODO: here if we sleep (block thread) for 1 sec it crashes ... why ?
				if (messagesToSend.try_dequeue(msgToSend)) {
					sendChatMessage(msgToSend.message);
				}

				keepConnectionAlive(connectionToServer);
			}

			if (!isRemoteAlive(connectionToServer))
				receivedMessages.emplace(TcpMsgType::END_OF_THREAD, "The remote host is not alive (timeout expired).");
		}
		catch (std::exception const& e) {
			std::cerr << e.what() << std::endl;
			receivedMessages.emplace(TcpMsgType::END_OF_THREAD, std::string("Exception: ") + e.what());
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