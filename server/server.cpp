#include "socket.h"

using namespace std;

int main() {
	//WSADATA ws = initializeWinsock();

	//SOCKET nSocket = initializeSocket();

	//sockaddr_in server = initializeServerSocket();

	//bindAndListen(nSocket, server);

	//SOCKET clientSocket = acceptRequestFromClient(nSocket);

	//processRequests(clientSocket, nSocket);

	//closeConnection(clientSocket, nSocket);

    WSADATA ws = initializeWinsock();

    // Tạo socket
    SOCKET nSocket = initializeSocket();

    // Cấu hình server socket
    sockaddr_in server = initializeServerSocket();

    // Liên kết và bắt đầu lắng nghe
    bindAndListen(nSocket, server);

    // Chạy broadcast trên luồng riêng
    thread broadcastThread(sendBroadcast);
    broadcastThread.detach();

    // Chấp nhận kết nối từ client
    SOCKET clientSocket = acceptRequestFromClient(nSocket);

    // Nhận và gửi các gói tin
    processRequests(clientSocket, nSocket);

    // Đóng phiên kết nối
    closeConnection(clientSocket, nSocket);

	return 0;
}
