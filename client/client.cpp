#include "lib.h"

int main() {

	WSADATA ws = initializeWinsock();

	SOCKET clientSocket = initializeSocket();

	sockaddr_in server = initializeServerSocket();

	connectToServer(clientSocket, server);

	receiveAndSend(clientSocket);

	return 0;
 }