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

string processAboutListApps(json j) {
    if (!j.contains("content")) {
        cout << "Applications list is empty!" << endl;
        return "";
    }

    vector<string> list = j.at("content");
    //vector<string> list = j.at("content").get<vector<string>>();

    cout << "List of appications: " << endl;
    for (string app : list) {
        cout << app << endl;
    }

    // Trả về kết quả xem server có thực hiện thành công hay không
    return j.at("result");
}

string processAboutListServices(json j) {
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

string processRespond(string jsonRespond) {

    //cout << jsonRespond.size();
    //return "";

    json j = json::parse(jsonRespond);

    //cout << j;

    //cout << endl;

    //vector<string> temp = j.at("content");
    //cout << j.at("title") << endl;
    //cout << temp.size();

    //return "";

    string result = "";

    if (j.contains("title")) {
        if (j.at("title") == "startApp") {
            result = j.at("result");
        }

        else if (j.at("title") == "listApps") {
            result = processAboutListApps(j);
        }

        else if (j.at("title") == "listServices") {
            result = processAboutListServices(j);
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
                result = j.at("result");
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
            result = j.at("result");
        }

        else if (j.at("title") == "stopWebcam") {
            result = j.at("result");
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