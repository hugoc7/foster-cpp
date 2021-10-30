#include "Server.h"


void Server::handleFullyReceivedPacket(int clientID) {

	connections[clientID].bytesReceived = 0;
	try {
		//interpret the buffer content to create a msg object
		TCPmessage newMessage(connections[clientID].buffer, connections[clientID].packetSize);

		switch (newMessage.type) {
			//if client request disconnection
			case TcpMsgType::DISCONNECTION_REQUEST:

				//player name is moved because the client struct will be destroyed
				newMessage.playerName = std::move(connections[clientID].playerName);
				assert(connections[clientID].playerID >= 0);
				newMessage.playerID = connections[clientID].playerID;
				connections[clientID].connectedToGame = false;
				closeConnection(clientID);
				break;

			case TcpMsgType::CONNECTION_REQUEST:
				connections[clientID].playerName = newMessage.playerName;
				connections[clientID].connectedToGame = true;
				connections[clientID].playerID = generatePlayerID();
				newMessage.playerID = connections[clientID].playerID;
				break;
			case TcpMsgType::SEND_CHAT_MESSAGE:
				newMessage.playerName = connections[clientID].playerName;
				assert(connections[clientID].playerID >= 0);
				newMessage.playerID = connections[clientID].playerID;
				break;
		}
		messages.lockQueue();
		messages.enqueue(std::move(newMessage));
		messages.unlockQueue();
	}
	catch (std::runtime_error& e) {
		closeConnection(clientID);
	}
}

void Server::closeConnection(int clientID) {
	if (connections[clientID].connectedToGame) {
		TCPmessage disconnectionMsg(TcpMsgType::DISCONNECTION_REQUEST, 
			std::move(connections[clientID].playerName), connections[clientID].playerID);
		messages.lockQueue();
		messages.enqueue(std::move(disconnectionMsg));//TODO: it would be better to EMPLACE (moodyCamelQueue has it)
		messages.unlockQueue();
		connections[clientID].connectedToGame = false;
	}
	connections[clientID].close();//close the socket
	removeFromVector(connections, clientID);
}

///@brief receive TCP messages from all clients (non blocking)
void Server::receiveMessagesFromClients(std::vector<ClientConnection>& clients, ConcurrentCircularQueue<TCPmessage>& messages) {

	//for each client
	for (int i = 0; i < clients.size(); i++) {


		if (!SDLNet_SocketReady(clients[i].tcpSocket)) continue;
		int ret;
		//if we already know the packet size
		if (clients[i].bytesReceived >= 2) {
			ret = SDLNet_TCP_Recv(clients[i].tcpSocket, clients[i].buffer.get() + clients[i].bytesReceived,
				clients[i].packetSize - clients[i].bytesReceived);
			if (ret == -1) {
				std::cout << "Error in SDLNet_TCP_Recv: " << SDLNet_GetError() << std::endl;
				closeConnection(i);
				continue;
			}
			clients[i].bytesReceived += ret;
		}

		//if we dont know the packet size yet
		else {
			ret = SDLNet_TCP_Recv(clients[i].tcpSocket, clients[i].buffer.get() + clients[i].bytesReceived, 
				clients[i].bufferSize - clients[i].bytesReceived);
			if (ret == -1) {
				std::cout << "Error in SDLNet_TCP_Recv: " << SDLNet_GetError() << std::endl;
				closeConnection(i);
				continue;
			}
			clients[i].bytesReceived += ret;

			//if we have received the packet size header
			if (clients[i].bytesReceived >= 2) {
				clients[i].packetSize = (Uint16)SDLNet_Read16(clients[i].buffer.get());
				//assert(clients[i].packetSize <= MAX_TCP_PACKET_SIZE, "The TCP packet is too big");
				//assert(clients[i].bytesReceived <= clients[i].packetSize, "The TCP packet is too short");
				if (clients[i].packetSize > MAX_TCP_PACKET_SIZE) {
					std::cout << "The TCP packet is too big, closing connection.\n";
					closeConnection(i);
					continue;
				}
				if (clients[i].bytesReceived > clients[i].packetSize) {
					std::cout << "The TCP packet is too short, closing connection.\n";
					closeConnection(i);
					continue;
				}

				//grow up the buffer if needed
				if (clients[i].bufferSize < clients[i].packetSize) {
					clients[i].bufferSize = clients[i].packetSize;
					clients[i].buffer.realloc(clients[i].bufferSize);
				}
			}
		}
		
		//packet is fully received
		if (clients[i].bytesReceived == clients[i].packetSize) {
			handleFullyReceivedPacket(i);
		}
	}
}


void Server::acceptNewConnection(TCPsocket listeningTcpSock) {
	// accept a connection coming in on server_tcpsock
	TCPsocket new_tcpsock{ NULL };

	if (!SDLNet_SocketReady(listeningTcpSock)) return;
	new_tcpsock = SDLNet_TCP_Accept(listeningTcpSock);
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
			connections.emplace_back(new_tcpsock, socketSet);
		}
	}
}

void Server::loop(TCPsocket listeningTcpSock, ConcurrentCircularQueue<TCPmessage>& msgQueue) {

	while (serverRunning) {


		if (connections.size() < MAX_SOCKETS)
			acceptNewConnection(listeningTcpSock);

		if (SDLNet_CheckSockets(socketSet, 0) == -1 && connections.size() > 0) {
			printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
			//most of the time this is a system error, where perror might help you.
			perror("SDLNet_CheckSockets");
			throw;
		}

		if (connections.size() > 0) {
			receiveMessagesFromClients(connections, msgQueue);
		}
	}

}
	
void Server::stop() {
	serverRunning = false;//std::atomic is noexcept
	thread.join();

	connections.clear();
	if (SDLNet_TCP_DelSocket(socketSet, listeningTcpSock) == -1) {
		printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
		// perhaps the socket is not in the set
		std::abort();
	}
	SDLNet_TCP_Close(listeningTcpSock);
	SDLNet_FreeSocketSet(socketSet);
}


void Server::start() {
	if (serverRunning) return;

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
	thread = std::thread(&Server::loop, this, listeningTcpSock, std::ref(messages));
}

Server::Server() : messages(50) {
	//start();
}
Server::~Server() {
	stop();
}

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
