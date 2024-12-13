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

sockaddr_in receiveBroadcast() {
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        cout << "Failed to create UDP socket: " << WSAGetLastError() << endl;
        exit(EXIT_FAILURE);
    }
    cout << "UDP socket created for receiving broadcast" << endl;

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(9909); // Cổng broadcast
    clientAddr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(udpSocket, (sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        cout << "Failed to bind UDP socket: " << WSAGetLastError() << endl;
        closesocket(udpSocket);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    sockaddr_in senderAddr;
    int senderLen = sizeof(senderAddr);

    cout << "Waiting for broadcast message..." << endl;
    int recvBytes = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
        (sockaddr*)&senderAddr, &senderLen);
    if (recvBytes < 0) {
        cout << "Failed to receive broadcast: " << WSAGetLastError() << endl;
        closesocket(udpSocket);
        exit(EXIT_FAILURE);
    }

    buffer[recvBytes] = '\0';
    cout << "Received broadcast message: " << buffer << endl;

    // Phân tích gói tin để lấy IP và cổng
    sockaddr_in server;
    server.sin_family = AF_INET;

    string message(buffer);
    size_t ipPos = message.find("Server_IP=");
    size_t portPos = message.find(";Port=");

    if (ipPos != std::string::npos && portPos != std::string::npos) {
        std::string serverIP = message.substr(ipPos + 10, portPos - (ipPos + 10));
        int serverPort = std::stoi(message.substr(portPos + 6));
        server.sin_addr.s_addr = inet_addr(serverIP.c_str());
        server.sin_port = htons(serverPort);

        cout << "Parsed server address: " << serverIP << ":" << serverPort << endl;
    }

    closesocket(udpSocket);
    return server;
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
            sendBuffer = "endProgram";
            send(clientSocket, sendBuffer.c_str(), sendBuffer.length(), 0);
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

    cout << "Successfully saved file to " << savePath << endl;
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
        string filePath = "D:\\test screenshot client\\testListApps.txt";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "listServices") {
        string filePath = "D:\\test screenshot client\\testListServices.txt";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
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
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "shutdown") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
    }

    else if (title == "startWebcam") {
        string filePath = "D:\\test screenshot client\\testVideo.avi";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

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
        string filePath = "D:\\test screenshot client\\picture_from_server.png";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        cout << j["result"] << endl;
    }

    else if (title == "sendFile") {
        string filePath = "D:\\test screenshot client\\sendFile.pdf";
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
        string filePath = "D:\\test screenshot client\\keyLogger.txt";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

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
    else if (title == "getDirectoryTree") {
        string filePath = "D:\\test screenshot client\\DirectoryTree.txt";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, filePath);

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