#pragma once
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
#include "json.hpp"

#pragma comment(lib, "Ws2_32.lib")

// Khai báo biến toàn cục
extern HHOOK keyboardHook;

using namespace std;
using json = nlohmann::json;

// Các hàm tạo môi trường socket để server và client giao tiếp

WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in initializeServerSocket();

void bindAndListen(SOCKET& nSocket, sockaddr_in& server);

SOCKET acceptRequestFromClient(SOCKET nSocket);

void processRequests(SOCKET& clientSocket, SOCKET& nSocket);

// Hàm ngắt kết nối với client
void closeConnection(SOCKET clientSocket, SOCKET nSocket);

// Hàm xử lý các yêu cầu của client
void processRequest(SOCKET& clinetSocket, string jsonRequest);

// Hàm xử lý chức năng gửi file
bool sendFile(SOCKET& clientSocket, const string& fileName);

// Hàm xử lý chức năng gửi messages bằng file JSON
void sendJSON(SOCKET& clientSocket, const string& jsonResponse);

// Hàm xử lý chức năng start app
string startApp(string name);

// Hàm xử lý chức năng list app
string listApps();
vector<string> createListApps();

// Các hàm xử lý chức năng list services
string listServices();

vector<string> createListServices();

// Hàm xử lý chức năng tắt máy
string shutdown();

// Hàm xử lý chức năng khởi động lại máy
string restart();

// Hàm xử lý chức năng xóa file
string deleteFile(string& filePath);

// Các hàm xử lý chức năng chụp màn hình

string generateFileName();

HBITMAP captureScreen(int& screenWidth, int& screenHeight);

bool saveHBitmapToBMP(HBITMAP hBitmap, int width, int height, const string& folderPath);

bool saveBinaryToImage(const string& binaryData, const string& savePath);

string screenShot();

// Các hàm xử lý chức năng keyLocking

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

bool LockKeyboard();

string solveKeyLockingAndSend(bool& flag);

void runKeyLockingLoop();

// Xử lý chức năng mở khóa keyLocking
string UnlockKeyboard();


// Xử lý chức năng keyLogger

map<int, string> createKeyMap();

vector<string> collectKeyNames(int durationInSeconds);
