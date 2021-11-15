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
	connections[clientID].close(socketSet);//close the socket
	removeFromVector(connections, clientID);
	sendPlayerDisconnectionNotification(playerID);
}

///@brief receive TCP messages from all clients (non blocking)
void Server::receiveMessagesFromClients() {
	//for each client
	for (int clientID = 0; clientID < connections.size(); clientID++) {
		try {
			if (receivePacket(connections[clientID])) {
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


void Server::acceptNewConnection() {
	// accept a connection coming in on server_tcpsock
	TCPsocketObject new_tcpsock;

	if (!listeningTcpSock.isReady()) return;//strange, it works but cheksockets have not been called yet

	if (listeningTcpSock.accept(new_tcpsock)) {

		/*std::cout << "New connection accepted with " << new_tcpsock.getPeerIP() 
			<< " on his port " << new_tcpsock.getPeerPort() << std::endl;*/
		connections.emplace_back(std::move(new_tcpsock), std::ref(socketSet));//error here
	}
}

void Server::loop() {

	while (serverRunning) {

		checkSockets(TCP_SLEEP_DURATION);


		if (connections.size() < MAX_SOCKETS)
			acceptNewConnection();


		if (connections.size() > 0) {
			receiveMessagesFromClients();
		}
	}

}
	
void Server::stop() {
	serverRunning = false;//std::atomic is noexcept
	thread.join();

	connections.clear();
	socketSet.delSocket(listeningTcpSock);
	listeningTcpSock.close();
	//SDLNet_FreeSocketSet(socketSet);
}


void Server::start(Uint16 port) {
	if (serverRunning) return;

	// create a listening TCP socket  (server)
	IPaddressObject ip;

	ip.resolveHost(port);
	listeningTcpSock.open(ip);
	socketSet.addSocket(listeningTcpSock);

	std::cout << "Starting server ..." << std::endl;
	serverRunning = true;
	thread = std::thread(&Server::loop, this);
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
