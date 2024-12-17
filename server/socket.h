#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <cstdio> 
#include <cstdlib>
#include <sstream>
#include <set>
#include <map>
#include <ctime>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in initializeServerSocket();

void bindAndListen(SOCKET& nSocket, sockaddr_in& server);

SOCKET acceptRequestFromClient(SOCKET nSocket);

void processRequests(SOCKET& clientSocket, SOCKET& nSocket);

void closeConnection(SOCKET& clientSocket, SOCKET& nSocket);

void processRequest(SOCKET&, string);

void sendResponse(SOCKET&, string);

