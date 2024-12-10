#include "socket.h"
#include "mail.h"
#include "commands.h"
#include "response.h"


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
    //server.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (InetPton(AF_INET, L"172.16.0.60", &server.sin_addr) != 1) {
        cout << "Failed to convert IP address" << endl;
        // Handle error appropriately
    }
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

void closeConnection(SOCKET clientSocket) {
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
    
	// connect to imap server to receive command from email
    imaps* imapConn = createIMAPConnection();

	// connect to smtp server to send response email
	smtps* smtpConn = createSMTPConnection();


    //message msg;
    ////msg.from(mail_address(SERVICE_MAIL_NAME, SERVICE_MAIL_ADDRESS));
    ////msg.add_recipient(mail_address(USER_MAIL_NAME, USER_MAIL_ADDRESS));
    ////msg.subject("Start App Response");
    ////msg.content("Connected to server successfully");

    //createMsg(msg);
    //sendMail(*smtpConn, msg);
    ////smtpConn->submit(createMessage("Connected to server successfully"));

    //exit(1);

    while (true) {
        string title = "";
        string nameObject = ""; // dùng chung cho mọi chức năng
        string source = "";
        string destination = ""; // dùng riêng cho chức năng copy file
        
		// receive command from email
        if (!receivedNewCommand(*imapConn, title, nameObject, source, destination)) continue;

        cout << "Processing " << title << " " << nameObject << endl;
        cout << endl;

		// check if user want to end program
        if (title == END_PROGRAM) {
            message msg;
			createMsg(msg, "End Program Response", "Program is ended");
			sendMail(*smtpConn, msg);
            closeConnection(clientSocket);
			break;
        }

		// create request to send to server
        sendBuffer = createRequest(title, nameObject, source, destination);

		// send the request to the server
        int sendResult = send(clientSocket, sendBuffer.c_str(), sendBuffer.length(), 0);
        if (sendResult == SOCKET_ERROR) {
            cout << "Failed to send data to server. Error: " << WSAGetLastError() << endl;
            break;
        }

		// get response from server and send email to user
        message msg;
        msg.content_transfer_encoding(mailio::mime::content_transfer_encoding_t::BASE_64);
        processResponse(msg, title, clientSocket);
        sendMail(*smtpConn, msg);
        //processResponse(title, clientSocket);
        
        cout << endl;
        //break;
    }

	// close connection to imap server
    delete imapConn;

	// close connection to smtp server
	delete smtpConn;
}

string createRequest(const string& title,const string& nameObject, const string& source, const string& destination) {
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
    string result = "";
    if (!j.contains("content")) {
        result = "Applications list is empty!\n";
        return result;
    }

    vector<string> list = j.at("content");

    result = "List of appications: \n";
    for (string app : list) {
		result += app + "\n";
    }

    // Trả về kết quả xem server có thực hiện thành công hay không
    return result + (string)j.at("result");
}

string processListServices(json j) {
    string result = "";
    if (!j.contains("content")) {
        result = "Services list is empty!\n";
        return result;
    }

    vector<string> list = j.at("content");

    result = "List of services: \n";
    for (string app : list) {
        result += app + "\n";
    }

    // Trả về kết quả xem server có thực hiện thành công hay không
    return result + (string)j.at("result");

}