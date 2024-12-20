#include "process.h"

void processEmailRequests(SOCKET& clientSocket) {
    string sendBuffer;
    string receiveBuffer;
    receiveBuffer.resize(1024);

    // connect to imap server to receive command from email
    imaps* imapConn = createIMAPConnection();

    // connect to smtp server to send response email
    smtps* smtpConn = createSMTPConnection();

    while (true) {
        string title = "";
        string nameObject = "";
        string source = "";
        string destination = "";

        // receive command from email
        if (!receivedNewCommand(*imapConn, title, nameObject, source, destination)) continue;

        cout << "Processing " << title <<
            //" " << nameObject << " " <<
            //source << " " << destination << 
            endl;

        // create request to send to server
        sendBuffer = createRequest(title, nameObject, source, destination);

        // send the request to the server
        int sendResult = send(clientSocket, sendBuffer.c_str(), (int)sendBuffer.length(), 0);
        if (sendResult == SOCKET_ERROR) {
            cout << "Failed to send data to server. Error: " << WSAGetLastError() << endl;
            message msg;
            createMessage(msg, "Socket Response", "Failed to send data to server");
            sendMail(*smtpConn, msg);
            break;
        }
        cout << "Request sent to server successfully" << endl;

        // check if user want to end program
        if (title == END_PROGRAM) {
            message msg;
            createMessage(msg, "End Program Response", "Program is ended");
            sendMail(*smtpConn, msg);
            break;
        }

        if (title == RESTART) {
            message msg;
            createMessage(msg, "Restart Response", "Your computer will be restart in few seconds");
            sendMail(*smtpConn, msg);
            break;
        }

        if (title == SHUTDOWN) {
            message msg;
            createMessage(msg, "Shutdown Response", "Your computer has been shutdown successfully");
            sendMail(*smtpConn, msg);
            break;
        }

        // get response from server and send email to user
        message msg;
        msg.content_transfer_encoding(mailio::mime::content_transfer_encoding_t::BASE_64);
        createReponseToUser(msg, title, nameObject, clientSocket);
        sendMail(*smtpConn, msg);
        cout << "Email response sent to user successfully" << endl << endl;
    }

    // close connection to imap server
    delete imapConn;

    // close connection to smtp server
    delete smtpConn;
}

string createRequest(const string& title, const string& nameObject, const string& source, const string& destination) {
    json j;

    if (title != "") j["title"] = title;

    if (j["title"] == "copyFile") {
        j["source"] = source;
        j["destination"] = destination;
    }

    else if (nameObject != "") j["nameObject"] = nameObject;

    return j.dump();
}

string receiveResponseFromServer(SOCKET& clientSocket) {
    // retrieve JSON size
    int jsonSize = 0;
    int result = recv(clientSocket, reinterpret_cast<char*>(&jsonSize), sizeof(jsonSize), 0);

    if (result == SOCKET_ERROR) {
        cout << "Failed to receive JSON size, error: " << WSAGetLastError() << endl;
        return "";
    }

    // check valid size (maximum 10MB)
    if (jsonSize <= 0 || jsonSize > 10 * 1024 * 1024) {
        cout << "Invalid JSON size received." << endl;
        return "";
    }

    // retrieve JSON
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

    cout << "Receive response from server successfully" << endl;
    return jsonBuffer;
}

void receiveFileFromServer(SOCKET& clientSocket, string fileName) {
    int fileSize = 0;
    int result = recv(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

    if (result == SOCKET_ERROR || fileSize <= 0) {
        cout << "Failed to receive file size, error: " << WSAGetLastError() << endl;
        return;
    }

    cout << "Receiving file of size: " << fileSize << " bytes" << endl;

    string binaryData;
    binaryData.resize(fileSize);

    int bytesReceived = 0;
    while (bytesReceived < fileSize) {
        int bytesToReceive = min(fileSize - bytesReceived, 1024);
        result = recv(clientSocket, &binaryData[bytesReceived], bytesToReceive, 0);

        if (result == SOCKET_ERROR) {
            cout << "Failed to receive file data, error: " << WSAGetLastError() << endl;
            return;
        }

        bytesReceived += result;
    }

    cout << "File received successfully" << endl;

    ofstream outFile(fileName, ios::binary);
    outFile.write(binaryData.c_str(), binaryData.size());
    outFile.close();
}

void createReponseToUser(message& msg, string title, string arg, SOCKET& clientSocket) {
    string response;
    json j;

    if (title == START_APP) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Start App Response", body);
    }

    else if (title == STOP_APP) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Stop App Response", body);
    }

    else if (title == LIST_APP) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "List App Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, DATA_FILE);
            attachFile(msg, "AppList.txt");
		}
    }

    else if (title == START_SERVICE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Start Service Response", j.at("result"));
    }

    else if (title == STOP_SERVICE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Stop Service Response", j.at("result"));
    }

    else if (title == LIST_SERVICE) {
        receiveFileFromServer(clientSocket, DATA_FILE);

        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "List Services Response", j.at("result"));
        attachFile(msg, "ServiceList.txt");
    }

    else if (title == LIST_PROCESS) {
        receiveFileFromServer(clientSocket, DATA_FILE);

        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "List Processes Response", j.at("result"));
        attachFile(msg, "ProcessList.txt");
    }

    else if (title == GET_FILE) {
        receiveFileFromServer(clientSocket, DATA_FILE);

        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);
        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Get File Response", j.at("result"));
        attachFile(msg, extractFileName(arg));
    }

    else if (title == COPY_FILE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);
        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Copy File Response", j.at("result"));
    }

    else if (title == DELETE_FILE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Delete File Response", j.at("result"));
    }

    else if (title == TAKE_SCREENSHOT) {
        receiveFileFromServer(clientSocket, DATA_FILE);

        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);
        
        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Screen Shot Response", j.at("result"));
        attachFile(msg, "screenshot.png");
    }

    else if (title == KEYLOGGER) {
        receiveFileFromServer(clientSocket, DATA_FILE);

        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Keylogger Response", j.at("result"));
		attachFile(msg, "Keylogger.txt");
    }

    else if (title == LOCK_KEYBOARD) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Key Lock Response", j.at("result"));
    }

    else if (title == UNLOCK_KEYBOARD) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Key Unlock Response", j.at("result"));
    }

    else if (title == LIST_DIRECTORY_TREE) {
        receiveFileFromServer(clientSocket, DATA_FILE);

		response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Directory Tree Response", j.at("result"));
		attachFile(msg, "DirectoryTree.txt");
    }

    else if (title == TURN_ON_WEBCAM) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Start Webcam Response", j.at("result"));
    }

    else if (title == TURN_OFF_WEBCAM) {
        receiveFileFromServer(clientSocket, VIDEO_FILE);

        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Stop Webcam Response", j.at("result"));
        attachVideo(msg, "record.mp4");
    }

    else {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Socket Response", j.at("result"));
    }
}