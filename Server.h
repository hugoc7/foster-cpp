#pragma once


#include "SDL_net.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include "ReaderWriterQueue.h"
#include "Containers.h"
#include "Network.h"
#include <atomic>






void dummy() {

	moodycamel::ReaderWriterQueue<int> q(100);       // Reserve space for at least 100 elements up front

	q.enqueue(17);                       // Will allocate memory if the queue is full
	bool succeeded = q.try_enqueue(18);  // Will only succeed if the queue has an empty slot (never allocates)
	assert(succeeded);

	int number;
	succeeded = q.try_dequeue(number);  // Returns false if the queue was empty
	assert(succeeded && number == 17);

	// You can also peek at the front item of the queue (consumer only)
	int* front = q.peek();
	assert(*front == 18);
	succeeded = q.try_dequeue(number);
	assert(succeeded && number == 18);
	front = q.peek();
	assert(front == nullptr);           // Returns nullptr if the queue was empty

}


struct ClientConnection {
	TCPsocket tcpSocket{ NULL };
	//std::string playerName{};//is it really useful ?
	
	std::unique_ptr<char[]> buffer{};
	int bufferSize{ 4 };//buffer size MUST always be >= 2
	int bytesReceived{ 0 };
	Uint16 packetSize{};


	ClientConnection(TCPsocket tcpSocket) : tcpSocket{ tcpSocket }, bufferSize{ 4 }, buffer(std::make_unique<char[]>(4)) {
	}
	ClientConnection(ClientConnection&& other) :
		tcpSocket{ std::move(other.tcpSocket) },
		buffer{ std::move(other.buffer) },
		bufferSize{ std::move(other.bufferSize) },
		bytesReceived{ std::move(other.bytesReceived) },
		packetSize{ std::move(other.packetSize) }
	{
		other.tcpSocket = NULL;
	}
	~ClientConnection() {
		if(tcpSocket != NULL)
			SDLNet_TCP_Close(tcpSocket);
	}
	ClientConnection& operator=(ClientConnection&& other) {
		tcpSocket = std::move(other.tcpSocket);
		buffer = std::move(other.buffer);
		bufferSize = std::move(other.bufferSize);
		bytesReceived = std::move(other.bytesReceived);
		packetSize = std::move(other.packetSize);
		other.tcpSocket = NULL;
		return *this;
	}
};



std::string receiveStringFromTCPsocket(TCPsocket tcpSock) {

	//first receive packet size (2 bytes = Uint6)
	char packetSizeBuffer[2];
	int bytesReceived = { 0 };
	do {
		bytesReceived += SDLNet_TCP_Recv(tcpSock, packetSizeBuffer + bytesReceived, 2 - bytesReceived);
	} while (bytesReceived < 2);
	Uint16 size = (Uint16)SDLNet_Read16(packetSizeBuffer);
	assert(size < 65536);

	//then receive packet content of arbitrary size
	char* buffer = new char[size];
	bytesReceived = 0;
	do {
		bytesReceived += SDLNet_TCP_Recv(tcpSock, buffer + bytesReceived, size - bytesReceived);
	} while (bytesReceived < size);

	std::string str;
	str.assign(buffer, size);
	delete[] buffer;
	return str;
}


class Server {
public:
	std::vector<ClientConnection> connections;
	std::chrono::milliseconds delayForNewConnection{ 500 };
	//moodycamel::ReaderWriterQueue<TCPmessage> messages;//concurrent queue
	ConcurrentCircularQueue<TCPmessage> messages;
	std::thread thread{};
	std::atomic<bool> serverRunning{ false };
	TCPsocket listeningTcpSock{ NULL };
	SDLNet_SocketSet socketSet{ NULL };
	const int MAX_SOCKETS{16};

	///@brief receive TCP messages from all clients (non blocking)
	void receiveMessagesFromClients(std::vector<ClientConnection>& clients, ConcurrentCircularQueue<TCPmessage>& messages) {

		//for each client
		for (int i = 0; i < clients.size(); i++) {

			ClientConnection& client{ clients[i] };//should be removed

			if (!SDLNet_SocketReady(client.tcpSocket)) continue;
			//receive packet size (2 byte)
			if (client.bytesReceived < 2) {
				int ret = SDLNet_TCP_Recv(client.tcpSocket, client.buffer.get() + client.bytesReceived, 2 - client.bytesReceived);
				if (ret == -1) {
					std::cout << SDLNet_GetError() << std::endl;
				}
				client.bytesReceived += ret;
			}

			if (client.bytesReceived == 2) {
				client.packetSize = (Uint16)SDLNet_Read16(client.buffer.get());
				assert(client.packetSize <= MAX_TCP_PACKET_SIZE, "The TCP packet is too big");
				//grow up the buffer if needed
				if (client.bufferSize < client.packetSize) {
					client.bufferSize = client.packetSize;
					client.buffer.reset(new char[client.bufferSize]);
				}
			}

			//receive packet content (n bytes)
			if (client.bytesReceived >= 2 && client.bytesReceived < 2 + client.packetSize) {
				int ret = SDLNet_TCP_Recv(client.tcpSocket, client.buffer.get() + client.bytesReceived - 2,
					client.packetSize - client.bytesReceived + 2);
				if (ret == -1) {
					std::cout << SDLNet_GetError() << std::endl;
				}
				client.bytesReceived += ret;
			}

			//packet is fully received
			if (client.bytesReceived == 2 + client.packetSize) {
				client.bytesReceived = 0;
				TCPmessage newMessage(client.buffer, client.packetSize, i);

				//if client request disconnection
				if (newMessage.type == TcpMsgType::DISCONNECTION_REQUEST) {

					if (SDLNet_TCP_DelSocket(socketSet, client.tcpSocket) == -1) {
						printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
						// perhaps the socket is not in the set
						exit(1);
					}
					removeFromVector(clients, i);
				}
				messages.lockQueue();
				messages.enqueue(std::move(newMessage));
				messages.unlockQueue();
			}
		}
	}

	void acceptNewConnection(TCPsocket listeningTcpSock) {
		// accept a connection coming in on server_tcpsock
		TCPsocket new_tcpsock{ NULL };
		
		if (!SDLNet_SocketReady(listeningTcpSock)) return;
		new_tcpsock = SDLNet_TCP_Accept(listeningTcpSock);
		std::cout << "Server socket: " << listeningTcpSock << std::endl;
		if (new_tcpsock != NULL) {

			// get the remote IP and port
			IPaddress* remote_ip{ NULL };
			remote_ip = SDLNet_TCP_GetPeerAddress(new_tcpsock);
			if (!remote_ip) {
				printf("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
				printf("This may be a server socket.\n");
			}
			else {
				std::cout << "New connection accepted with " << getIpAdressAsString(remote_ip->host) << " on his port " << remote_ip->port << std::endl;
				//free(remote_ip);
				if (SDLNet_TCP_AddSocket(socketSet, new_tcpsock) == -1) {
					printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
					// perhaps you need to restart the set and make it bigger...
					exit(1);
				}
				else {
					connections.emplace_back(new_tcpsock);
				}
			}
		}	
	}

	void create_sockset(SDLNet_SocketSet set)
	{
		if (set) {
			SDLNet_TCP_DelSocket(set, listeningTcpSock);
			if(connections.size() > 0)
				SDLNet_TCP_DelSocket(set, connections[0].tcpSocket);

			SDLNet_FreeSocketSet(set);
		}
		set = SDLNet_AllocSocketSet(MAX_SOCKETS);
		if (!set) {
			printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
			exit(1); /*most of the time this is a major error, but do what you want. */
		}
		SDLNet_TCP_AddSocket(set, listeningTcpSock);
		for (int i = 0; i < connections.size(); i++)
			SDLNet_TCP_AddSocket(set, connections[i].tcpSocket);
	}

	void loop(TCPsocket listeningTcpSock, ConcurrentCircularQueue<TCPmessage>& msgQueue) {
		while (serverRunning) {

			//create_sockset(socketSet);

			if (connections.size() < MAX_SOCKETS)
				acceptNewConnection(listeningTcpSock);

			if (SDLNet_CheckSockets(socketSet, 1000) == -1 && connections.size() > 0) {
				printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
				//most of the time this is a system error, where perror might help you.
				perror("SDLNet_CheckSockets");
				exit(1);//here we should throw an exception instead !
			}

			if (connections.size() > 0) {
				
				receiveMessagesFromClients(connections, msgQueue);

			}
		}
	}
	/*void loop(int a, std::string b) {
		std::cout << "yolo";
	}*/
	
	void stop() {
		if (!serverRunning)
			return;

		serverRunning = false;//DATA RACE HERE
		thread.join();
		connections.clear();
		if (SDLNet_TCP_DelSocket(socketSet, listeningTcpSock) == -1) {
			printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
			// perhaps the socket is not in the set
			//throw exception
			exit(1);
		}
		SDLNet_TCP_Close(listeningTcpSock);
		SDLNet_FreeSocketSet(socketSet);
	}
	

	void start() {
		if (serverRunning.load()) return;

		// create a listening TCP socket on port 9999 (server)
		IPaddress ip;
		

		socketSet = SDLNet_AllocSocketSet(MAX_SOCKETS);

		if (!socketSet) {
			printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
			exit(1); //most of the time this is a major error, but do what you want.
		}

		if (SDLNet_ResolveHost(&ip, NULL, 9999) == -1) {
			std::cout << "SDLNet_ResolveHost: " << SDLNet_GetError() << std::endl;
			exit(1);
		}

		listeningTcpSock = SDLNet_TCP_Open(&ip);
		if (!listeningTcpSock) {
			std::cout << "SDLNet_TCP_Open: " << SDLNet_GetError() << std::endl;
			exit(2);
		}
		if (SDLNet_TCP_AddSocket(socketSet, listeningTcpSock) == -1) {
			printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
			// perhaps you need to restart the set and make it bigger...
			//throw exception
			exit(1);
		}

		std::cout << "Starting server ..." << std::endl;
		serverRunning = true;
		//std::thread t(&Server::loop, this, listeningTcpSock, std::ref(messages));
		//thread = std::move(t);
		loop(listeningTcpSock, messages);
	}

	Server() : messages(50) {
		//start();
	}
	~Server() {
		stop();
	}

};
