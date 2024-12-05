#include <iostream>
#include <string>
#include <winsock.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

string buildRequest(const string& title, const string& content, const string& nameObject, const string& result);

string processRespond(string jsonRespond);

string processAboutListApps(json j);

string processAboutListServices(json j);