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
#include <set>
#include "json.hpp"

using namespace std; 
using json = nlohmann::json;

WSADATA initializeWinsock();

SOCKET initializeSocket();

sockaddr_in initializeServerSocket();

void bindAndListen(SOCKET& nSocket, sockaddr_in& server);

SOCKET acceptRequestFromClient(SOCKET nSocket);

void ReceiveAndSend(SOCKET& clientSocket, SOCKET& nSocket);

void closeConected(SOCKET clientSocket, SOCKET nSocket);

// Process request from client
string processRequest(string jsonRequest);

// Process start app function
string startApp(string name);

// Process list apps function
string listApps();
vector<string> createListApps();

// Process list services function
string listServices();
vector<string> createListServices();

//string convertListToJsonString(vector<string> listApps);




