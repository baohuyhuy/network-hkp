#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <winsock.h>
#include <windows.h>
#include <cstdio> // cho _popen và _pclose
#include <cstdlib>
#include <sstream>
#include "json.hpp"

using namespace std; 
using json = nlohmann::json;

string startApp(string name);

vector<string> createListApps();

//string convertListToJsonString(vector<string> listApps);

string processRequest(string jsonRequest);
