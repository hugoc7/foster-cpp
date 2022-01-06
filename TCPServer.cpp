#include "TCPServer.h"

void TCPServer::closeConnection(int clientID) {
	Uint16 playerID{ connections[clientID].playerID };
	if (connections[clientID].connectedToGame) {
		messages.emplace(std::make_unique<DisconnectionRequest>(std::move(connections[clientID].playerName), connections[clientID].playerID));
		connections[clientID].connectedToGame = false;//useless ?
		releasePlayerID(connections[clientID].playerID);
	}
	try_releaseRecvBuffer(connections[clientID]);
	connections[clientID].close(socketSet);//close the socket
	removeFromVector(connections, clientID);
	sendPlayerDisconnectionNotification(playerID);//ici pb à gérer!!!
}

///@brief receive TCP messages from all clients (non blocking)
void TCPServer::receiveMessagesFromClients() {
	//for each client
	for (int clientID = 0; clientID < connections.size(); clientID++) {
		try {
			if (receivePacket(connections[clientID])) {
				connections[clientID].bytesReceived = 0;

				//interpret the buffer content to create a msg object
				std::unique_ptr<TCPMessage> newMessage = TCPMessage::ReadFromBuffer(getRecvBuffer(connections[clientID].recvBufferIndex), connections[clientID].packetSize);

				releaseRecvBuffer(connections[clientID]);


				switch (newMessage.get()->type) {
					//if client request disconnection
				case TcpMsgType::DISCONNECTION_REQUEST: {
					DisconnectionRequest* disconnectionMsg = static_cast<DisconnectionRequest*>(newMessage.get());

					//player name is moved because the client struct will be destroyed
					disconnectionMsg->playerName = std::move(connections[clientID].playerName);
					assert(connections[clientID].playerID >= 0);
					disconnectionMsg->playerID = connections[clientID].playerID;
					connections[clientID].connectedToGame = false;

					releasePlayerID(connections[clientID].playerID);
					closeConnection(clientID);
					messages.enqueue(std::move(newMessage));
					break;
				}

				case TcpMsgType::CONNECTION_REQUEST: {

					/*if (connectedClients++ > MAX_CLIENTS) {
						sendGoodbye(connections[clientID], "This server is full (too much players).");
						closeConnection(clientID);//TODO: pb here
						continue;
					}*/
					ConnectionRequest* connectionMsg = static_cast<ConnectionRequest*>(newMessage.get());
					connections[clientID].playerName = connectionMsg->playerName;
					connections[clientID].connectedToGame = true;
					Uint16 playerID = getNewPlayerID();
					connections[clientID].playerID = playerID;

					assert(playerID < playerIdToConnectionIndex.size());
					playerIdToConnectionIndex[playerID] = clientID;

					connectionMsg->playerID = playerID;
					connectionMsg->clientIp = std::move(connections[clientID].tcpSocket.getPeerIP());
					messages.enqueue(std::move(newMessage));
					break;
				}

				case TcpMsgType::SEND_CHAT_MESSAGE: {
					SendChatMsg* chatMsg = static_cast<SendChatMsg*>(newMessage.get());
					chatMsg->playerName = connections[clientID].playerName;
					assert(connections[clientID].playerID >= 0);
					chatMsg->playerID = connections[clientID].playerID;
					sendChatMessage(chatMsg->message, chatMsg->playerID);
					messages.enqueue(std::move(newMessage));
					break; 
				}
				case TcpMsgType::STILL_ALIVE:
					//nothing
					break;
				}
			}
		}
		catch (std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			//try to say goodbye ...
			try {
				sendGoodbye(connections[clientID], std::string("Exception raised: ") + e.what());
			}
			catch(...){}

			//TODO: ici on a un probleme si une exception survient lors de l'envoi d'une notification
			//à un autre client (dans closeConnection), il faudrait pouvoir fermer la connexion recursivement
			//gros dossier ....
			closeConnection(clientID);
		}
	}
}


void TCPServer::acceptNewConnection() {
	// accept a connection coming in on server_tcpsock
	TCPsocketObject new_tcpsock;

	if (!listeningTcpSock.isReady()) return;//strange, it works but cheksockets have not been called yet


	if (listeningTcpSock.accept(new_tcpsock)) {

		std::cout << "New connection accepted with " << new_tcpsock.getPeerIP() 
			<< " on his port " << new_tcpsock.getPeerPort() << std::endl;
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

			handleMessagesFromGame();//is it the good place ?


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
		std::cerr << "\nException in server thread: " << e.what() << std::endl;
		messages.emplace(std::make_unique<EndOfThread>(std::string("Exception: ") + e.what()));

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
}


void TCPServer::start(Uint16 port, Uint16 udpPort) {
	if (serverRunning) return;

	udpServerPort = udpPort;

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
	availablePlayerIDs.reserve(MAX_CLIENTS);
	playerIdToConnectionIndex.resize(MAX_CLIENTS);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		availablePlayerIDs.push_back(MAX_CLIENTS - 1 - i);
	}
}
TCPServer::~TCPServer() {
	stop();
}