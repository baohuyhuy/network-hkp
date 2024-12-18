#pragma once
//#include <iostream>
//#include <vector>
//#include <string>
//#include <windows.h>
//#include <cstdio> 
//#include <cstdlib>
//#include <sstream>
//#include <set>
//#include <map>
//#include <ctime>
//#include <thread>
//#include <ws2tcpip.h> 

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <cstdio> 
#include <cstdlib>
#include <sstream>
#include <set>
#include <map>
#include <ctime>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in initializeServerSocket();

void bindAndListen(SOCKET& nSocket, sockaddr_in& server);

void sendBroadcast();

SOCKET acceptRequestFromClient(SOCKET nSocket);

void processRequests(SOCKET& clientSocket, SOCKET& nSocket);

void closeConnection(SOCKET& clientSocket, SOCKET& nSocket);

void processRequest(SOCKET&, string);

void sendResponse(SOCKET&, string);

