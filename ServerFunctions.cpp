#include "Functions.h"

string startApp(string name) {
    string command = "start " + name;
	int result = system(command.c_str());

    json jsonRespond;

    jsonRespond["title"] = "startApp";
    jsonRespond["nameObject"] = name;

    if (result == 0)
        jsonRespond["result"] = "Successfully started " + name;
	else
        jsonRespond["result"] = "Failed to start " + name;

    return jsonRespond.dump();
}

string listApps() {
    vector<string> list = createListApps();

    json jsonRespond;

    jsonRespond["title"] = "listApps";
    
    if (list.size() != 0) {
        jsonRespond["result"] = "Successfully listed applications";
        jsonRespond["content"] = list;
    }
    else
        jsonRespond["result"] = "Failed to list applications ";

    return jsonRespond.dump();
}

vector<string> createListApps() {
    string command = "powershell -Command \"gps | where {$_.mainwindowhandle -ne 0} | select-object name\"";

    // Sử dụng hàm _popen để đọc file từ lệnh command ở trên
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        cerr << "Failed to run command." << endl;
        return {};
    }

    // Tạo vector để lưu danh sách các ứng dụng đang hoạt động
    vector<string> listApps;

    // Tạo một buffer để đọc dữ liệu theo từng dòng
    char buffer[256];


    // Đọc dữ liệu bằng stringstream
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        istringstream ss(buffer);
        string appName;

        // Lấy tên ứng dụng ở dòng hiện tại
        ss >> appName;

        // Bỏ qua dòng tiêu đề nếu dòng hiện tại là "Name" hoặc "----"
        if (appName == "Name" || appName == "----") {
            continue;
        }

        // Kiểm tra tên ứng dụng có hợp lệ không
        if (!appName.empty()) {
            listApps.push_back(appName);
        }
    }

    _pclose(pipe);

    return listApps;
}

string processRequest(string jsonRequest) {
    // Tạo đối tượng JSON từ chuỗi JSON
    json j = json::parse(jsonRequest);

    string result = "";

    if (j.contains("title")) {
        if (j.at("title") == "listApps") {
                result = listApps();
        }
        else if (j.at("title") == "listServices") {
            // process services
        }
        else if (j.at("title") == "startApp") {
            if (j.contains("nameObject")) {
                result = startApp(j.at("nameObject"));
            }
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
