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

wchar_t* wstringToWcharArray(const std::wstring& wstr) {
    wchar_t* wcharArray = new wchar_t[wstr.size() + 1];
    std::copy(wstr.begin(), wstr.end(), wcharArray);
    wcharArray[wstr.size()] = L'\0'; // Null-terminate the array
    return wcharArray;
}

sockaddr_in initializeServerSocket() {
    sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(9909);
	wchar_t* serverIP = wstringToWcharArray(SERVER_IP);
    if (InetPton(AF_INET, serverIP, &server.sin_addr) != 1) {
        cout << "Failed to convert server's IP address" << endl;
        // Handle error appropriately
		exit(EXIT_FAILURE);
    }
    delete serverIP;
    return server;
}

void connectToServer(SOCKET clientSocket, sockaddr_in server) {
    int connectResult = connect(clientSocket, (sockaddr*)&server, sizeof(server));

    if (connectResult < 0) {
        cout << "Failed to connect to server" << endl;
		exit(EXIT_FAILURE);
    }
    cout << "Connected to server successfully" << endl;
	cout << "Start receiving and sending data to server" << endl;
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

    while (true) {
        string title = "";
        string nameObject = ""; // dùng chung cho mọi chức năng
        string source = "";
        string destination = ""; // dùng riêng cho chức năng copy file
        
		// receive command from email
        if (!receivedNewCommand(*imapConn, title, nameObject, source, destination)) continue;

        cout << "Processing " << title << " " << nameObject << " " 
            << source << " " << destination << endl;
        cout << endl;

		// check if user want to end program
        if (title == END_PROGRAM) {
            message msg;
			createMessage(msg, "End Program Response", "Program is ended");
			sendMail(*smtpConn, msg);
            closeConnection(clientSocket);
			break;
        }

        if (title == RESTART) {
            message msg;
            createMessage(msg, "Restart Response", "Your computer will be restart in few seconds");
            sendMail(*smtpConn, msg);
        }

        if (title == SHUTDOWN) {
			message msg;
			createMessage(msg, "Shutdown Response", "Your computer has been shutdown successfully");
			sendMail(*smtpConn, msg);
        }

		// create request to send to server
        sendBuffer = createRequest(title, nameObject, source, destination);

		// send the request to the server
        int sendResult = send(clientSocket, sendBuffer.c_str(), (int) sendBuffer.length(), 0);
        if (sendResult == SOCKET_ERROR) {
            cout << "Failed to send data to server. Error: " << WSAGetLastError() << endl;
            break;
        }

		// get response from server and send email to user
        message msg;
        msg.content_transfer_encoding(mailio::mime::content_transfer_encoding_t::BASE_64);
        processResponse(msg, title, nameObject, clientSocket);
        sendMail(*smtpConn, msg);
        
        cout << endl;
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

string receiveResponse(SOCKET& clientSocket) {
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
