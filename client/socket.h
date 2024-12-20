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

// socket functions for connect and close connection between client and server
WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in receiveBroadcast();

void connectToServer(SOCKET clientSocket, sockaddr_in server);

void closeConnection(SOCKET clientSocket);

