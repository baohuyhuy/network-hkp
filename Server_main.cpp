#include "Server_library.h"
#include "Server_functions.h"

using namespace std;

int main() {

	/*WSADATA ws = initializeWinsock();

	SOCKET nSocket = initializeSocket();

	sockaddr_in server = initializeServerSocket();

	bindAndListen(nSocket, server);*/

	//SOCKET clientSocket = acceptRequestFromClient(nSocket);

	//ReceiveAndSend(clientSocket, nSocket);

	//closeConected(clientSocket, nSocket);

	handleServer();

	/*string IP = getLocalIPAddress();
	cout << IP;*/

	return 0;
}

