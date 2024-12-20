#include "socket.h"
#include "process.h"

int main() {
	WSADATA ws = initializeWinsock();

	SOCKET clientSocket = initializeSocket();

	sockaddr_in server = receiveBroadcast();

	connectToServer(clientSocket, server);

	processEmailRequests(clientSocket);

	closeConnection(clientSocket);

	return 0;
 }
