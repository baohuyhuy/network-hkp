#pragma once

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <mailio/imap.hpp>

using namespace std;
using mailio::message;
using mailio::codec;
using mailio::imaps;
using mailio::imap_error;
using mailio::dialog_error;

imaps* createIMAPConnection();

bool receivedNewCommand(imaps&, string&, string&, string&, string&);

bool getNewMessage(imaps&, message&);

string getMessageTextBody(message&);