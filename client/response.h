#pragma once

#include <string>
#include "socket.h"
#include "mail.h"
#include ".env.h"
#include "commands.h"

using namespace std;

void processResponse(message& msg, string, SOCKET&);