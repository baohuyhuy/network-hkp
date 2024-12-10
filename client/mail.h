#pragma once

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <mailio/imap.hpp>
#include <mailio/smtp.hpp>
#include <mailio/message.hpp>

using namespace std;
using mailio::message;
using mailio::codec;
using mailio::imaps;
using mailio::imap_error;
using mailio::dialog_error;
using mailio::mail_address;
using mailio::smtps;
using mailio::smtp_error;
using mailio::string_t;

// receive mail
imaps* createIMAPConnection();

bool receivedNewCommand(imaps&, string&, string&, string&, string&);

bool getNewMail(imaps&, message&);

string getMessageTextBody(message&);

// send mail
smtps* createSMTPConnection();

void sendMail(smtps&, message&);

message& createMessage(string);

void createMsg(message&, string, string);

void attachFile(message&, const string&);