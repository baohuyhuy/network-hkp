#include "socket.h"

int main() {
	WSADATA ws = initializeWinsock();

	SOCKET clientSocket = initializeSocket();

	//sockaddr_in server = initializeServerSocket();
	sockaddr_in server = receiveBroadcast();

	connectToServer(clientSocket, server);

	receiveAndSend(clientSocket);

	closeConnection(clientSocket);

	//WSADATA ws = initializeWinsock();

	//sockaddr_in server = receiveBroadcast();

	//SOCKET clientSocket = initializeSocket();

	////sockaddr_in server = initializeServerSocket();

	//connectToServer(clientSocket, server);

	//receiveAndSend(clientSocket);

	//closesocket(clientSocket);

	//WSACleanup();

	return 0;

	return 0;
 }
