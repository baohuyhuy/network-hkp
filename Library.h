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
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fcntl.h>
#include <io.h>
#include <filesystem>
#include <locale>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <opencv2/core/utils/logger.hpp>
#include "json.hpp"

#pragma comment(lib, "Ws2_32.lib")

#define MAX_THREADS 15

// Khai báo biến toàn cục lưu giữ hook bàn phím
extern HHOOK keyboardHook;

// Queue công việc (Task Queue) và các thông tin liên quan
extern std::vector<std::thread> workers;                 // Danh sách các luồng
extern std::queue<std::function<void()>> tasks;          // Hàng đợi công việc
extern std::mutex resourceMutex;                         // Cơ chế đồng bộ hóa
extern std::mutex queueMutex;                            // Mutex bảo vệ hàng đợi
extern std::condition_variable condition;                // Condition variable
extern std::atomic<bool> stop;                           // Cờ dừng Thread Pool

//extern std::thread messageLoopThread;

using namespace std;
using json = nlohmann::json;