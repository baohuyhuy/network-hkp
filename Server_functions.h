#pragma once
#include "Library.h"

// Các hàm tạo môi trường socket để server và client giao tiếp

WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in initializeServerSocket();

void bindAndListen(SOCKET& nSocket, sockaddr_in& server);

SOCKET acceptRequestFromClient(SOCKET nSocket);

void ReceiveAndSend(SOCKET& clientSocket, SOCKET& nSocket);

void handleServer();

void initializeThreadPool(size_t maxThreads);

void addTaskToQueue(const std::function<void()>& task);

void shutdownThreadPool();

// Hàm ngắt kết nối với client
void closeConected(SOCKET clientSocket, SOCKET nSocket);

// Hàm xử lý các yêu cầu của client
void processRequest(SOCKET& clientSocket, string jsonRequest);

// Hàm xử lý chức năng gửi file
bool sendFile(SOCKET& clientSocket, const string& fileName);

// Hàm xử lý chức năng gửi messages bằng file JSON
void sendJSON(SOCKET& clientSocket, const string& jsonResponse);

// Hàm xử lý chức năng start app
string startApp(string name);

// Hàm xử lý chức năng list app
string listApps();
void writeAppListToFile(const vector<vector<string>>& apps);
vector<vector<string>> getRunningApps();

//vector<string> createListApps();

// Các hàm xử lý chức năng list services
string listServices();
std::string ConvertWideCharToString(LPCWSTR wideCharStr);
string GetServiceDescription(SC_HANDLE hService);
string TruncateString(const string& str, size_t maxLength);
vector<pair<string, tuple<int, string, string>>> ListAllServices();
void writeServicesListToFile(const vector<pair<string, tuple<int, string, string>>>& services);

//vector<string> createListServices();

// Hàm xử lý chức năng bật webcam
string startWebcam(SOCKET clientSocket, int duration);

// Hàm xử lý chức năng tắt webcam
string stopWebcam();

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
bool UnlockKeyboard();

string solveKeyUnlockingAndSend(bool& flag);

// Xử lý chức năng keyLogger

map<int, string> createKeyMap();

vector<string> collectKeyNames(int durationInSeconds);

// Xử lý chức năng getDirectoryTree

bool isHiddenOrSystem(const std::filesystem::path& path);

void printDirectoryTree(const std::filesystem::path& path, std::wofstream& output, int indent, int currentDepth, int maxDepth);

bool listDrivesAndPrintTree();

string createDiractoryTree();
