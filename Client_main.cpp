#include "Client_functions.h"

int main() {

	WSADATA ws = initializeWinsock();

	sockaddr_in server = receiveBroadcast();

	SOCKET clientSocket = initializeSocket();

	//sockaddr_in server = initializeServerSocket();

	connectToServer(clientSocket, server);

	receiveAndSend(clientSocket);

	closesocket(clientSocket);

	WSACleanup();

	return 0;
 }