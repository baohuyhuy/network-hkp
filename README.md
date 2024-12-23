# PC Control by Email

This project is a part of the 24-25 23CTT2 HCMUS Network Computer Project.

## Description

PC Control by Email is a system that allows users to control their PC remotely via email commands. The project is written in C++, using window socket for client-server communication and [mailio](https://github.com/karastojko/mailio) for sending and receiving email.

## Features

- Remote control of PC via email
- Command execution
- Email notifications for command execution status

## Set up

### Prerequisites

#### Client
- A PC running Windows
- Visual Studio
- C++ 17 compiler
- vcpkg [(see how to install)](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell)

#### Server
- A PC running Windows (can be the same PC as client)
- Visual Studio
- C++ 17 compiler
- vcpkg [(see how to install)](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell)

#### A valid email account
- Note for Gmail users: it might be needed to register mailio as a trusted application by create an app password for it.
  
### Installing

#### Client
1. Create new project in Visual Studio
2. Add all files in [client folder](https://github.com/baohuyhuy/pc-control-by-email/tree/main/client) to project
3. Create `.env.h` file with this snippet:
```cpp
#pragma once

#include <string>

using namespace std;

const string SERVICE_MAIL_NAME = "Your Service Address Name";
const string SERVICE_MAIL_ADDRESS = "your_service_address@mail.com";
const string SERVICE_MAIL_PASSWORD = "your app password";

const string IMAP_SERVER = "imap.mailserver.com"; // example: imap.gmail.com
const int IMAP_PORT = 000; // example: 993

const string SMTP_SERVER = "smtp.mailserver.com"; // example: smtp.gmail.com
const int SMTP_PORT = 000; // example: 587

const string USER_MAIL_NAME = "Your User Address Name";
const string USER_MAIL_ADDRESS = "your_user_address@mail.com";
// replace example with your real email
```

#### Server
1. Create new project in Visual Studio
2. Add all files in [server folder](https://github.com/baohuyhuy/pc-control-by-email/tree/main/server) to project

### Usage

- Run server project before client project.
- Send an email with the desired command to the specified email address.
