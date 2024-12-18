#pragma once

#ifdef _WIN32
#  include <WinSock2.h> // Include WinSock2.h before Windows.h or SDKDDKVer.h
#  ifdef USE_ASIO
//     Set the proper SDK version before including boost/Asio
#      include <SDKDDKVer.h>
//     Note boost/ASIO includes Windows.h. 
#      include <boost/asio.hpp>
#   else //  USE_ASIO
#      include <Windows.h>
#   endif //  USE_ASIO
#else // _WIN32
#  ifdef USE_ASIO
#     include <boost/asio.hpp>
#  endif // USE_ASIO
#endif //_WIN32

#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <nlohmann/json.hpp>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
using json = nlohmann::json;

// Socket environment functions for communication between client and server
WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in receiveBroadcast();

void connectToServer(SOCKET clientSocket, sockaddr_in server);

void closeConnection(SOCKET clientSocket);

// Build a request from the user to send to the server
string createRequest(const string& title, const string& nameObject, const string& source, const string& destination);

// Function to receive a response and send a message to the server
void processEmailRequests(SOCKET clientSocket);

// Function to save binary data to a file
void saveBinaryToFile(const string& binaryData, const string& savePath);

// Function to handle receiving a file
string receiveFile(SOCKET& clientSocket);

// Function to handle receiving data as a JSON file
string receiveResponse(SOCKET& clientSocket);


