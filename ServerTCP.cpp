#include "Functions.h"

using namespace std;

int main() {

	// Initialize the winsock
	//WSADATA ws;
	//if (WSAStartup(MAKEWORD(2, 2), &ws) < 0) {
	//	cout << "WSA failed to initialize" << endl;
	//	return EXIT_FAILURE;
	//}
	//else
	//	cout << "WSA initialized" << endl;

	WSADATA ws = initializeWinsock();

	// Initialize the socket
	//SOCKET nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	//if (nSocket < 0) {
	//	cout << "The Socket not opened" << endl;
	//}
	//else
	//	cout << "The Socket opened successfully" << endl;

	SOCKET nSocket = initializeSocket();

	// Initilize the envỉoment for sockaddr structure
	//sockaddr_in server;

	//server.sin_family = AF_INET;
	//server.sin_port = htons(9909);
	////server.sin_addr.s_addr = inet_addr("127.0.0.1");
	//server.sin_addr.s_addr = INADDR_ANY;
	//memset(&(server.sin_zero), 0, 8);

	sockaddr_in server = initializeServerSocket();

	// Bind the socket to the local host
	//int nRet = 0;
	//nRet = ::bind(nSocket, (sockaddr*)&server, sizeof(sockaddr));

	//if (nRet < 0) {
	//	cout << "Failed to bind to local port" << endl;
	//	return EXIT_FAILURE;
	//}
	//else
	//	cout << "Successfully bind to local port" << endl;

	//// Listen the request from client 
	//nRet = listen(nSocket, 5);
	//if (nRet < 0) {
	//	cout << "Failed to start listen to local port" << endl;
	//}
	//else
	//	cout << "Started listening to local port" << endl;

	bindAndListen(nSocket, server);

	// Accept the request from client 
	//SOCKET clientSocket;
	//sockaddr_in client;
	//int clientSize = sizeof(client);

	//clientSocket = accept(nSocket, (sockaddr*)&client, &clientSize);

	//if (clientSocket < 0) {
	//	cout << "Failed to accept connection" << endl;
	//	return EXIT_FAILURE;
	//}
	//else
	//	cout << "Client connected successfully" << endl;

	SOCKET clientSocket = acceptRequestFromClient(nSocket);

	//// Receive and respond messages from client 

	//string receiveBuffer, sendBuffer;
	//receiveBuffer.resize(512);

	ReceiveAndSend(clientSocket, nSocket);

	//// Receive request from client
	//int recvResult = recv(clientSocket, &receiveBuffer[0], receiveBuffer.size(), 0);

	//if (recvResult <= 0) {
	//	cout << "Failed to receive data from client" << endl;
	//}
	//else {
	//	receiveBuffer.resize(recvResult);

	//	sendBuffer = processRequest(receiveBuffer);

	//	//cout << "Received from client: " << receiveBuffer << endl;

	//	// Send messages to client 
	//	send(clientSocket, sendBuffer.c_str(), sendBuffer.size(), 0);

	//	// close connected with client
	//	//closesocket(clientSocket);

	//	// close socket server
	//	//closesocket(nSocket);
	//	//WSACleanup();

	//	return 0;
	//}

	return 0;
}

