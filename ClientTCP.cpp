#include "Functions.h"

int main() {
	// Initialize winsock
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws) < 0) {
		cout << "WSA failed to initialize" << endl;
	}
	else
		cout << "WSA initialized" << endl;

	// Initialize Socket
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (clientSocket < 0) {
		cout << "Failed to create client socket" << endl;
		return EXIT_FAILURE;
	}
	else {
		cout << "Client socket created successfully" << endl;
	}

	// Initilize the envỉoment for sockaddr structure
	sockaddr_in server;
	
	server.sin_family = AF_INET;
	server.sin_port = htons(9909);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Connect to server
	int connectResult = connect(clientSocket, (sockaddr*)&server, sizeof(server));
	
	if (connectResult < 0) {
		cout << "Failed to connect to server" << endl;
		return EXIT_FAILURE;
	}
	else
		cout << "Connected to server successfully" << endl;

	// Send message to server
	string sendBuffer, receiveBuffer;
	receiveBuffer.resize(512);


	//cout << "Client send: ";
	//getline(cin, sendBuffer);

	sendBuffer = buildRequest("listApps", "", "", "");
	send(clientSocket, sendBuffer.c_str(), sendBuffer.size(), 0);

	// Receive messages from server
	int receiveResult = recv(clientSocket, &receiveBuffer[0], receiveBuffer.size(), 0);

	if (receiveResult < 0) {
		cout << "Failed to receive data from server" << endl;
	}
	else {
		receiveBuffer.resize(receiveResult);
		cout << "Received from server: " << processRespond(receiveBuffer) << endl;
	}


	// Close the connection
	closesocket(clientSocket);
	WSACleanup();

	return 0;
 }