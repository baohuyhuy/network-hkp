#include "Functions.h"

string buildRequest(const string& title, const string& content, const string& nameObject, const string& result) {
    json j;

    // Gán các giá trị vào các key tương ứng
    if(title != "") j["title"] = title;
    if(content != "") j["content"] = content;
    if (nameObject != "") j["nameObject"] = nameObject;
    if(result != "") j["result"] = result;

    // Trả về chuỗi JSON
    return j.dump(); // Chuyển đổi đối tượng JSON thành chuỗi
}

vector<string> processAboutListApps(json j) {
    vector<string> list = j.at("content");
    return list;
}

string processRespond(string jsonRespond) {
    json j = json::parse(jsonRespond);
    string result = "";

    if (j.contains("title")) {
        if (j.at("title") == "startApps") {
            // process start App
            result = j.at("result");
        }

        else if (j.at("title") == "listApps") {
            // process list App

            vector<string> list = processAboutListApps(j);

            cout << "List of appications: " << endl;
            for (string app : list) {
                cout << app << endl;
            }

            result = j.at("result");
        }

        else if (j.at("title" == "listServices")) {
            // process list Services
        }

        else if (j.at("title") == "listServices") {
            // process services
        }

        else if (j.at("title") == "startService") {
            if (j.contains("nameObject")) {
                // process start service
            }
        }

        else if (j.at("title") == "stopApp") {
            if (j.contains("nameObject")) {
                //process stop app
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
            // process start webcam
        }

        else if (j.at("title") == "stopWebcam") {

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

    return result;
}