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
#include <ws2tcpip.h> // Để sử dụng inet_ntop
//#include <opencv4/opencv2/opencv.hpp>
#include <chrono>
//#include <opencv4/opencv2/core/utils/logger.hpp>
#include "json.hpp"

#pragma comment(lib, "Ws2_32.lib")

// Khai báo biến toàn cục lưu giữ hook bàn phím
extern HHOOK keyboardHook;

// Khai báo biến xem server có kết nối được tới client chưa
extern BOOL isConnected;

using namespace std;
using json = nlohmann::json;