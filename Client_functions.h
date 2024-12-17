#pragma once
#include "Library.h"
// Các hàm môi trường socket để giao tiếp giữa client và server

WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in receiveBroadcast();

sockaddr_in initializeServerSocket();

void connectToServer(SOCKET clientSocket, sockaddr_in server);

// Hàm đóng kết nối với server
void closedConected(SOCKET clientSocket);

// Xây dựng yêu cầu từ người dùng để gửi qua server
string buildRequest(const string& title, const string& nameObject, const string& source, const string& destination);

// Hàm nhận phản hồi và gửi thư qua cho server
void receiveAndSend(SOCKET clientSocket);

// Hàm xử lý phản hồi từ server
void processResponse(string title, SOCKET& clientSocket);

// Hàm lưu trữ dữ liệu nhị phân thành một file 
void saveBinaryToFile(const string& binaryData, const string& savePath);

// Hàm xử lý chức năng nhận file 
string receiveFile(SOCKET& clientSocket);

// Hàm xử lý chức năng nhận dữ liệu là file JSON 
string receiveJSON(SOCKET& clientSocket);


