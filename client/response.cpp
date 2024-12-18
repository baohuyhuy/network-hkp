#include "response.h"

void processResponse(message& msg, string title, string arg, SOCKET& clientSocket) {
    string response;
    json j;

    if (title == START_APP) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Start App Response", j.at("result"));
    }

    else if (title == LIST_APP) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "List Apps Response", j.at("result"));

        if (j["status"] == "OK") {
            string fileName = "receivedData.bin";
            string binaryData = receiveFile(clientSocket);
            saveBinaryToFile(binaryData, fileName);
            attachFile(msg, "AppList.txt");
		}
    }

    else if (title == LIST_SERVICE) {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "List Services Response", j.at("result"));
        attachFile(msg, "ServiceList.txt");
    }

    else if (title == LIST_PROCESS) {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "List Processes Response", j.at("result"));
        attachFile(msg, "ProcessList.txt");
    }

    else if (title == START_SERVICE) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Start Service Response", j.at("result"));
    }

    else if (title == STOP_APP) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Stop App Response", j.at("result"));
    }

    else if (title == STOP_SERVICE) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Stop Service Response", j.at("result"));
    }

    else if (title == TURN_ON_WEBCAM) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Start Webcam Response", j.at("result"));
    }

    else if (title == TURN_OFF_WEBCAM) {
        string fileName = "record.mp4";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Stop Webcam Response", j.at("result"));
        attachVideo(msg, "record.mp4");
    }

    else if (title == TAKE_SCREENSHOT) {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        response = receiveResponse(clientSocket);
        j = json::parse(response);
        
        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Screen Shot Response", j.at("result"));
        attachFile(msg, "screenshot.png");
    }

    else if (title == SEND_FILE) {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        response = receiveResponse(clientSocket);
        j = json::parse(response);
        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Send File Response", j.at("result"));
        attachFile(msg, extractFileName(arg));
    }

    else if (title == COPY_FILE) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);
        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Copy File Response", j.at("result"));
    }

    else if (title == DELETE_FILE) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Delete File Response", j.at("result"));
    }

    else if (title == KEYLOGGER) {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Keylogger Response", j.at("result"));
		attachFile(msg, "Keylogger.txt");
    }

    else if (title == LOCK_KEYBOARD) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Key Lock Response", j.at("result"));
    }

    else if (title == UNLOCK_KEYBOARD) {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Key Unlock Response", j.at("result"));
    }

    else if (title == LIST_DIRECTORY_TREE) {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

		response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << "Message from server: " << j["result"] << endl;
        createMessage(msg, "Directory Tree Response", j.at("result"));
		attachFile(msg, "DirectoryTree.txt");
    }

    // 
    else {
        response = receiveResponse(clientSocket);
        j = json::parse(response);

        cout << j["result"] << endl;
    }
}