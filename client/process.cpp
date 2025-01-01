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
		string newCommandResult = receivedNewCommand(*imapConn, title, nameObject, source, destination);
		if (newCommandResult == "NO") continue;
		cout << "Received new command: " << title << endl;
		if (newCommandResult == "ERROR") {
			cout << "Invalid command format" << endl;
			message msg;
			createMessage(msg, "Error Response", "Invalid command format");
			sendMail(*smtpConn, msg);
            cout << "Email response sent to user successfully" << endl << endl;
			continue;
		}

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

        message msg;
        msg.content_transfer_encoding(mailio::mime::content_transfer_encoding_t::BASE_64);
        // get response from server and send email to user
        createReponseToUser(msg, title, nameObject, clientSocket);
        sendMail(*smtpConn, msg);
        
        cout << "Email response sent to user successfully" << endl << endl;

		if (title == RESTART || title == SHUTDOWN || title == DISCONNECT) break;
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

bool receiveFileFromServer(SOCKET& clientSocket, string fileName) {
    int fileSize = 0;
    int result = recv(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

    if (result == SOCKET_ERROR || fileSize <= 0) {
        cout << "Failed to receive file size, error: " << WSAGetLastError() << endl;
        return false;
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
            return false;
        }

        bytesReceived += result;
    }

    cout << "File received successfully" << endl;

    ofstream outFile(fileName, ios::binary);
    outFile.write(binaryData.c_str(), binaryData.size());
    outFile.close();

	return true;
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
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Start Service Response", body);
    }

    else if (title == STOP_SERVICE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Stop Service Response", body);
    }

    else if (title == LIST_SERVICE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "List Service Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, DATA_FILE);
            attachFile(msg, "ServiceList.txt");
        }
    }

    else if (title == LIST_PROCESS) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "List Process Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, DATA_FILE);
            attachFile(msg, "ProcessList.txt");
        }
    }

    else if (title == GET_FILE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Get File Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, DATA_FILE);
            attachFile(msg, extractFileName(arg));
        }
    }

    else if (title == COPY_FILE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
		createMessage(msg, "Copy File Response", body);
    }

    else if (title == DELETE_FILE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Delete File Response", body);
    }

    else if (title == TAKE_SCREENSHOT) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Screen Shot Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, DATA_FILE);
            attachFile(msg, "screenshot.png");
        }
    }

    else if (title == KEYLOGGER) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Keylogger Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, DATA_FILE);
            attachFile(msg, "Keylogger.txt");
        }
    }

    else if (title == LOCK_KEYBOARD) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Lock Keyboard Response", body);
    }

    else if (title == UNLOCK_KEYBOARD) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Unlock Keyboard Response", body);
    }

    else if (title == RESTART) {
        json j;
        j["status"] = "OK";
        j["result"] = "Successfully disconnect from server";

        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];

        message msg;
        createMessage(msg, "Restart Response", body);
    }

    else if (title == SHUTDOWN) {
        json j;
        j["status"] = "OK";
        j["result"] = "Successfully disconnect from server";

        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];

        message msg;
        createMessage(msg, "Shutdown Response", body);
    }

    else if (title == LIST_DIRECTORY_TREE) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "List Directory Tree Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, DATA_FILE);
            attachFile(msg, "DirectoryTree.txt");
        }
    }

    else if (title == TURN_ON_WEBCAM) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Turn On Webcam Response", body);
    }

    else if (title == TURN_OFF_WEBCAM) {
        response = receiveResponseFromServer(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];
        createMessage(msg, "Turn Off Webcam Response", body);

        if (j["status"] == "OK") {
            receiveFileFromServer(clientSocket, VIDEO_FILE);
            attachVideo(msg, VIDEO_FILE);
        }
    }

    else if (title == DISCONNECT) {
        json j;
        j["status"] = "OK";
        j["result"] = "Successfully disconnect from server";

        string body =
            "Status: " + ((string)j["status"]) + "\n"
            "Message: " + (string)j["result"];

        createMessage(msg, "Disconnect Response", body);
    }



    //else {
    //    response = receiveResponseFromServer(clientSocket);
    //    j = json::parse(response);

    //    cout << "Message from server: " << j["result"] << endl;
    //    createMessage(msg, "Socket Response", j.at("result"));
    //}
}