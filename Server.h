#pragma once

#include <cstdio>
#include <cstdint>
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include "stdint.h"
#include "stdio.h"
#include <cstdlib>
#include <unordered_map>
#include <string>
#include <sstream>


enum class KReturn;

class Server
{
public:

	KReturn Init(uint16_t Port, uint32_t ChatCap);
	int send_all(SOCKET s, const char* msg, int len);
	 int send_packet(SOCKET s, const char* msg);
	 int revc_exact(SOCKET s, char* incomingMsg, int len);
	 int recv_packet(SOCKET s, char incomingMsg[256]);
	 KReturn server_commands(SOCKET s, const char* buffer);
	 bool LoggedIn(SOCKET s);
private:

	std::unordered_map<SOCKET, std::string> SocketToUser;
	std::unordered_map<std::string, SOCKET> UserToSocket;

	fd_set MasterSet;
	fd_set ReadySet;
	int MaxSocketNum;
};