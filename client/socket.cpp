#include "socket.h"
#include "mail.h"
#include "commands.h"
#include "process.h"


WSADATA initializeWinsock() {
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) < 0) {
        cout << "WSA failed to initialize" << endl;
    }

    return ws;
}

SOCKET initializeSocket() {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (clientSocket < 0) {
        cout << "Failed to create client socket" << endl;
        return EXIT_FAILURE;
    }
    else {
        cout << "Client socket created successfully" << endl;
    }

    return clientSocket;
}

wchar_t* stringToWcharArray(string& str) {
    size_t size = str.size() + 1;
    wchar_t* wcharArray = new wchar_t[size];
    mbstowcs_s(nullptr, wcharArray, size, str.c_str(), size - 1);
    return wcharArray;
}

sockaddr_in receiveBroadcast() {
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        cout << "Failed to create UDP socket: " << WSAGetLastError() << endl;
        exit(EXIT_FAILURE);
    }
    cout << "UDP socket created for receiving broadcast" << endl;

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(9909);
    clientAddr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(udpSocket, (sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        cout << "Failed to bind UDP socket: " << WSAGetLastError() << endl;
        closesocket(udpSocket);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    sockaddr_in senderAddr;
    int senderLen = sizeof(senderAddr);

    cout << "Waiting for broadcast message..." << endl;
    int recvBytes = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
        (sockaddr*)&senderAddr, &senderLen);
    if (recvBytes < 0) {
        cout << "Failed to receive broadcast: " << WSAGetLastError() << endl;
        closesocket(udpSocket);
        exit(EXIT_FAILURE);
    }

    buffer[recvBytes] = '\0';
    cout << "Received broadcast message: " << buffer << endl;

    sockaddr_in server;
    server.sin_family = AF_INET;

    string message(buffer);
    size_t ipPos = message.find("Server_IP=");
    size_t portPos = message.find(";Port=");

    if (ipPos != string::npos && portPos != string::npos) {
        string serverIP = message.substr(ipPos + 10, portPos - (ipPos + 10));
        int serverPort = std::stoi(message.substr(portPos + 6));
        
        wchar_t* serverIP2 = stringToWcharArray(serverIP);
        if (InetPton(AF_INET, serverIP2, &server.sin_addr) != 1) {
            cout << "Failed to convert server's IP address" << endl;
            exit(EXIT_FAILURE);
        }
        delete serverIP2;

        server.sin_port = htons(serverPort);

        cout << "Parsed server address: " << serverIP << ":" << serverPort << endl;
    }

    closesocket(udpSocket);
    return server;
}

void connectToServer(SOCKET clientSocket, sockaddr_in server) {
    int connectResult = connect(clientSocket, (sockaddr*)&server, sizeof(server));

    if (connectResult < 0) {
        cout << "Failed to connect to server" << endl;
		exit(EXIT_FAILURE);
    }
    cout << "Connected to server successfully" << endl;
	cout << "Start receiving and sending data to server" << endl << endl;
}

void closeConnection(SOCKET clientSocket) {
    // close the connection to the server
    int closeResult = closesocket(clientSocket);
    if (closeResult == 0) {
        cout << "Socket closed successfully" << endl;
    }
    else {
        cout << "Failed to close socket: " << WSAGetLastError() << endl;
    }

    // cleanup Winsock
    int cleanupResult = WSACleanup();
    if (cleanupResult == 0) {
        cout << "Winsock cleaned up successfully" << endl;
    }
    else {
        cout << "Failed to clean up Winsock: " << WSAGetLastError() << endl;
    }
}