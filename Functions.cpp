#include "Functions.h"

WSADATA initializeWinsock() {
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) < 0) {
        cout << "WSA failed to initialize" << endl;
    }
    else
        cout << "WSA initialized" << endl;

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
    else
        cout << "Successfully bind to local port" << endl;

    // Listen the request from client 
    nRet = listen(nSocket, 5);
    if (nRet < 0) {
        cout << "Failed to start listen to local port" << endl;
    }
    else
        cout << "Started listening to local port" << endl;
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
    else
        cout << "Client connected successfully" << endl;

    return clientSocket;
}

void ReceiveAndSend(SOCKET& clientSocket, SOCKET& nSocket) {
    string receiveBuffer, sendBuffer;
    receiveBuffer.resize(512);

    // Receive request from client
    int recvResult = recv(clientSocket, &receiveBuffer[0], receiveBuffer.size(), 0);

    if (recvResult <= 0) {
        cout << "Failed to receive data from client" << endl;
    }
    else {
        receiveBuffer.resize(recvResult);

        sendBuffer = processRequest(receiveBuffer);

        //cout << "Received from client: " << receiveBuffer << endl;

        // Send messages to client 
        send(clientSocket, sendBuffer.c_str(), sendBuffer.size(), 0);

        closeConected(clientSocket, nSocket);

        return;
    }
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

    json jsonRespond;

    jsonRespond["title"] = "startApp";
    jsonRespond["nameObject"] = name;

    if (result == 0)
        jsonRespond["result"] = "Successfully started " + name;
	else
        jsonRespond["result"] = "Failed to start " + name;

    return jsonRespond.dump();
}

string startWebcam() {
    string command = "start microsoft.windows.camera:";
    int result = system(command.c_str());

    json jsonRespond;

    jsonRespond["title"] = "startWebcam";
    
    if (result == 0)
        jsonRespond["result"] = "Successfully started webcam";
    else
        jsonRespond["result"] = "Failed to start webcam";

    return jsonRespond.dump();
}

string stopApp(string name) {
    string command = "taskkill /IM " + name + ".exe /F";
    int result = system(command.c_str());

    json jsonRespond;

    jsonRespond["title"] = "stopApp";
    jsonRespond["nameObject"] = name;

    if (result == 0)
        jsonRespond["result"] = "Successfully stopped " + name;
    else {
        //jsonRespond["result"] = "Failed to stop " + name;
        FILE* pipe = _popen(command.c_str(), "r");
        char buffer[128];
        string line = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            line += buffer;  // Đọc từng dòng đầu ra
        }
    }

    return jsonRespond.dump();
}

string stopWebcam() {
    string command = "powershell -Command \"Get-Process| Where-Object { $_.Name -eq 'WindowsCamera' } | Stop-Process\"";
    int result = system(command.c_str());

    json jsonRespond;

    jsonRespond["title"] = "stopWebcam";

    if (result == 0)
        jsonRespond["result"] = "Successfully stopped webcam";
    else
        jsonRespond["result"] = "Failed to stop webcam";

    return jsonRespond.dump();
}

string listApps() {
    vector<string> list = createListApps();

    json jsonRespond;

    jsonRespond["title"] = "listApps";
    
    if (list.size() != 0) {
        jsonRespond["result"] = "Successfully listed applications";
        jsonRespond["content"] = list;
    }
    else
        jsonRespond["result"] = "Failed to list applications";

    return jsonRespond.dump();
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

    json jsonRespond;

    jsonRespond["title"] = "listServices";

    if (list.size() != 0) {
        jsonRespond["result"] = "Successfully listed services";
        jsonRespond["content"] = list;
    }
    else
        jsonRespond["result"] = "Failed to list services";

    return jsonRespond.dump();
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

string processRequest(string jsonRequest) {
    // Tạo đối tượng JSON từ chuỗi JSON
    json j = json::parse(jsonRequest);

    string jsonRepond = "";

    if (j.contains("title")) {
        if (j.at("title") == "listApps") {
                jsonRepond = listApps();

                cout << jsonRepond << endl;
        }
        else if (j.at("title") == "listServices") {
            jsonRepond = listServices();

            cout << jsonRepond.size();

        }
        else if (j.at("title") == "startApp") {
            if (j.contains("nameObject")) {
                jsonRepond = startApp(j.at("nameObject"));
            }
        }
        else if (j.at("title") == "startService") {
            if (j.contains("nameObject")) {
                // process start service
            }
        }
        else if (j.at("title") == "stopApp") {
            if (j.contains("nameObject")) {
                jsonRepond = stopApp(j.at("nameObject"));
            }
        }
        else if (j.at("title") == "stopService") {
            if (j.contains("nameObject")) {
                //process stop service
            }
        }
        else if (j.at("title") == "restart") {
            // process restart
        }
        else if (j.at("title") == "shutdown") {
            // process shutdown
        }
        else if (j.at("title") == "startWebcam") {
            jsonRepond = startWebcam();
        }

        else if (j.at("title") == "stopWebcam") {
            jsonRepond = stopWebcam();
        }

        else if (j.at("title") == "screenShot") {
            // process screen shot
        }

        else if (j.at("title") == "copyFile") {
            // process copy file
        }

        else if (j.at("title") == "deleteFile") {
            // process delete file
        }

        else if (j.at("title") == "keyLogger") {
            // process keyLogger
        }

        else if (j.at("title") == "keyLocking") {
            // process keyLocking
        }
    }

    return jsonRepond;
}
