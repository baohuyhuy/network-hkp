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
#include "json.hpp"

#pragma comment(lib, "Ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;
using json = nlohmann::json;

// Các hàm môi trường socket để giao tiếp giữa client và server
WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in initializeServerSocket();

void connectToServer(SOCKET clientSocket, sockaddr_in server);

// Hàm đóng kết nối với server
void closeConnection(SOCKET clientSocket);

// Xây dựng yêu cầu từ người dùng để gửi qua cho server
string createRequest(const string& title, const string& nameObject, const string& source, const string& destination);

// Hàm nhận phản hồi và gửi thư qua cho server
void receiveAndSend(SOCKET clientSocket);

// Hàm xử lý phản hồi từ server
void processResponse(string title, SOCKET& clientSocket);

// Hàm xử lý chức năng list apps
string processListApps(json j);

// Hàm xử lý chức năng list services
string processListServices(json j);

// Hàm lưu trữ dữ liệu nhị phân thành một file 
void saveBinaryToFile(const string& binaryData, const string& savePath);

// Hàm xử lý chức năng nhận file 
string receiveFile(SOCKET& clientSocket);

// Hàm xử lý chức năng nhận dữ liệu là file JSON 
string receiveJSON(SOCKET& clientSocket);


