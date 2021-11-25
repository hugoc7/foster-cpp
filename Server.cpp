#include "TCPServer.h"

void TCPServer::closeConnection(int clientID) {
	Uint16 playerID{ connections[clientID].playerID };
	if (connections[clientID].connectedToGame) {
		messages.emplace(TcpMsgType::DISCONNECTION_REQUEST,
			std::move(connections[clientID].playerName), connections[clientID].playerID);
		connections[clientID].connectedToGame = false;
	}
	try_releaseRecvBuffer(connections[clientID]);
	connections[clientID].close(socketSet);//close the socket
	removeFromVector(connections, clientID);
	sendPlayerDisconnectionNotification(playerID);
}

///@brief receive TCP messages from all clients (non blocking)
void TCPServer::receiveMessagesFromClients() {
	//for each client
	for (int clientID = 0; clientID < connections.size(); clientID++) {
		try {
			if (receivePacket(connections[clientID])) {
				connections[clientID].bytesReceived = 0;

				//interpret the buffer content to create a msg object
				TCPmessage newMessage(getRecvBuffer(connections[clientID].recvBufferIndex), connections[clientID].packetSize);

				releaseRecvBuffer(connections[clientID]);


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
				case TcpMsgType::STILL_ALIVE:
					//nothing
					break;
				}
				//messages.lockQueue();
				messages.enqueue(std::move(newMessage));
				//messages.unlockQueue();
			}
		}
		catch (std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			sendGoodbye(connections[clientID], std::string("Exception raised: ") + e.what());
			closeConnection(clientID);
		}
	}
}


void TCPServer::acceptNewConnection() {
	// accept a connection coming in on server_tcpsock
	TCPsocketObject new_tcpsock;

	if (!listeningTcpSock.isReady()) return;//strange, it works but cheksockets have not been called yet

	if (listeningTcpSock.accept(new_tcpsock)) {

		/*std::cout << "New connection accepted with " << new_tcpsock.getPeerIP() 
			<< " on his port " << new_tcpsock.getPeerPort() << std::endl;*/
		connections.emplace_back(std::move(new_tcpsock), std::ref(socketSet));//error here
	}
}

void TCPServer::loop() {
	std::string stopReason{"Server has been stopped."};

	try {
		while (serverRunning) {

			checkSockets(TCP_SLEEP_DURATION);


			if (connections.size() < MAX_SOCKETS)
				acceptNewConnection();


			if (connections.size() > 0) {
				receiveMessagesFromClients();
			}

			for (int i = 0; i < connections.size(); i++) {
				//disconnect client when timeout is expired
				if (!isRemoteAlive(connections[i])) {
					sendGoodbye(connections[i], "Socket timeout expired");
					closeConnection(i);
				}

				keepConnectionAlive(connections[i]);
			}
		}
	}
	catch (std::exception const& e) {
		std::cerr << e.what() << std::endl;
		messages.emplace(TcpMsgType::END_OF_THREAD, std::string("Exception: ") + e.what());
		stopReason = e.what();
	}
	
	//we try to say goodbye to all client
	for (int i = 0; i < connections.size(); i++) {
		try {
		sendGoodbye(connections[i], stopReason);
		closeConnection(i);
		}
		catch (...) {
		}
	}
	
	//wait until the main thread close this thread
	while (serverRunning) {
		SDL_Delay(THREAD_CLOSING_DELAY);
	}
}
	
void TCPServer::stop() {
	if (!serverRunning) return;
	serverRunning = false;//std::atomic is noexcept
	thread.join();

	socketSet.delSocket(listeningTcpSock);
	listeningTcpSock.close();
	//SDLNet_FreeSocketSet(socketSet);
}


void TCPServer::start(Uint16 port) {
	if (serverRunning) return;

	// create a listening TCP socket  (server)
	IPaddressObject ip;

	ip.resolveHost(port);
	listeningTcpSock.open(ip);
	socketSet.addSocket(listeningTcpSock);

	std::cout << "Starting server ..." << std::endl;
	serverRunning = true;
	thread = std::thread(&TCPServer::loop, this);
}

TCPServer::TCPServer() : TCPNetworkNode(), messages(MAX_MESSAGES) {
	//start();
}
TCPServer::~TCPServer() {
	stop();
}