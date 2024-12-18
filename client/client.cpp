#include "socket.h"

int main() {
	WSADATA ws = initializeWinsock();

	SOCKET clientSocket = initializeSocket();

	sockaddr_in server = receiveBroadcast();

	connectToServer(clientSocket, server);

	// read the requests from the email and send it to the server
	processEmailRequests(clientSocket);

	closeConnection(clientSocket);

	return 0;
 }
