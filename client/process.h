#pragma once

#include "socket.h"
#include "mail.h"
#include ".env.h"
#include "commands.h"

using namespace std;

const string DATA_FILE = "receivedData.bin";
const string VIDEO_FILE = "record.mp4";

// create request from the user to send to server
string createRequest(const string&, const string&, const string&, const string&);

// receive request from user through email and forward it to server
void processEmailRequests(SOCKET&);

// handle receiving a file from server
void receiveFileFromServer(SOCKET& , string);

// handle receiving response message from server
string receiveResponseFromServer(SOCKET&);

// create response to send to user through email
void createReponseToUser(message&, string, string, SOCKET&);