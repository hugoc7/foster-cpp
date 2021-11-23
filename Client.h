#pragma once


#include <iostream>
#include <string>
#include "Network.h"
#include "Containers.h"
#include "TCPNetworkNode.h"
#include "SDLNetCpp.h"



class Client : TCPNetworkNode {
public:
	moodycamel::ReaderWriterQueue<TCPmessage> receivedMessages;
	moodycamel::ReaderWriterQueue<TCPmessage> messagesToSend;
	std::atomic<bool> clientRunning{ false };

	Client() : TCPNetworkNode(), receivedMessages(MAX_MESSAGES) {
	}

	~Client() {
		stop();
	}

	void start(std::string const& myName, std::string const& hostname = "127.0.0.1", Uint16 port = 9999) {
		if (clientRunning) return;

		playerName = myName;
		openConnectionToServer(hostname, port);
		clientRunning = true;
		thread = std::thread(&Client::loop, this);
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
			sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::DISCONNECTION_REQUEST, "");
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
			assert(sendingBuffer.Size() >= 4);
			SDLNet_Write16((Uint16)4, sendingBuffer.get());
			SDLNet_Write16((Uint16)type, sendingBuffer.get() + 2);
			sendPacket(connectionToServer, 4);
		}
		else {
			Uint16 size = str.size() + 4;
			assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
			if (sendingBuffer.Size() < size) {
				sendingBuffer.lossfulRealloc(size);
			}
			SDLNet_Write16((Uint16)size, sendingBuffer.get());
			SDLNet_Write16((Uint16)type, sendingBuffer.get() + 2);
			std::memcpy(sendingBuffer.get() + 4, str.c_str(), str.size());
			sendPacket(connectionToServer, size);
		}
	}

	void loop() {
		try {
			sendTCPmsg(connectionToServer.tcpSocket, TcpMsgType::CONNECTION_REQUEST, playerName);
			isConnected = true;

			TCPmessage msgToSend{};

			//TODO: the thread should be joinable so if the loop end, just send a message to the main thread and wait
			while (clientRunning && isRemoteAlive(connectionToServer)) {
				//here we sleep a bit in select syscall (10ms ?) to avoid crazy CPU usage
				checkSockets(TCP_SLEEP_DURATION);
				receiveMsg();

				//TODO: here if we sleep (block thread) for 1 sec it crashes ... why ?
				if (messagesToSend.try_dequeue(msgToSend)) {
					sendTCPmsg(connectionToServer.tcpSocket, msgToSend.type, msgToSend.message);
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