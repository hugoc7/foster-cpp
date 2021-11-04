#include "Server.h"

void Server::closeConnection(int clientID) {
	Uint16 playerID{ connections[clientID].playerID };
	if (connections[clientID].connectedToGame) {
		TCPmessage disconnectionMsg(TcpMsgType::DISCONNECTION_REQUEST, 
			std::move(connections[clientID].playerName), connections[clientID].playerID);
		//messages.lockQueue();
		messages.enqueue(std::move(disconnectionMsg));//TODO: it would be better to EMPLACE (moodyCamelQueue has it)
		//messages.unlockQueue();
		connections[clientID].connectedToGame = false;
	}
	connections[clientID].close();//close the socket
	removeFromVector(connections, clientID);
	sendPlayerDisconnectionNotification(playerID);
}

///@brief receive TCP messages from all clients (non blocking)
void Server::receiveMessagesFromClients(std::vector<ClientConnection>& clients, moodycamel::ReaderWriterQueue<TCPmessage>& messages) {
	//for each client
	for (int clientID = 0; clientID < clients.size(); clientID++) {
		try {
			if (receivePacket(clients[clientID])) {
				connections[clientID].bytesReceived = 0;
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
					sendPlayerList(connections[clientID]);
					sendNewPlayerNotification(connections[clientID]);
					break;

				case TcpMsgType::SEND_CHAT_MESSAGE:
					newMessage.playerName = connections[clientID].playerName;
					assert(connections[clientID].playerID >= 0);
					newMessage.playerID = connections[clientID].playerID;
					sendChatMessage(newMessage.message, newMessage.playerID);
					break;
				}
				//messages.lockQueue();
				messages.enqueue(std::move(newMessage));
				//messages.unlockQueue();
			}
		}
		catch (std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			closeConnection(clientID);
		}
	}
}


void Server::acceptNewConnection(TCPsocket listeningTcpSock) {
	// accept a connection coming in on server_tcpsock
	TCPsocket new_tcpsock{ NULL };

	if (!SDLNet_SocketReady(listeningTcpSock)) return;//strange, it works but cheksockets have not been called yet
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

void Server::loop(TCPsocket listeningTcpSock, moodycamel::ReaderWriterQueue<TCPmessage>& msgQueue) {

	while (serverRunning) {

		checkSockets(connections.size());


		if (connections.size() < MAX_SOCKETS)
			acceptNewConnection(listeningTcpSock);


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
	//SDLNet_FreeSocketSet(socketSet);
}


void Server::start() {
	if (serverRunning) return;

	// create a listening TCP socket on port 9999 (server)
	IPaddress ip;


	//socketSet = SDLNet_AllocSocketSet(MAX_SOCKETS);

	/*if (!socketSet) {
		printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
		exit(1); //most of the time this is a major error, but do what you want.
	}*/

	if (SDLNet_ResolveHost(&ip, NULL, 9999) == -1) {
		std::cout << "SDLNet_ResolveHost: " << SDLNet_GetError() << std::endl;
		exit(1);
	}

	listeningTcpSock = SDLNet_TCP_Open(&ip);
	if (!listeningTcpSock) {
		std::cout << "SDLNet_TCP_Open: " << SDLNet_GetError() << std::endl;
		exit(2);
	}
	//socketSet = SDLNet_AllocSocketSet(MAX_SOCKETS);
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

Server::Server() : TCPNetworkNode(), messages(MAX_MESSAGES), buffer(4), bufferSize{ 4 } {
	//start();
}
Server::~Server() {
	stop();
}

void sendToAll(TCPmessage&& message) {

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
