#include "Server.h"
#include "Returns .h"
#include "HashTable.h"
#include "ServerCommands.h"

bool Server::LoggedIn(SOCKET s)
{
	//if socket is not found in map return false
	return SocketToUser.find(s) != SocketToUser.end();
}

KReturn Server::Init(uint16_t port, uint32_t ChatCap)
{
	WSADATA WsaData;
	int ErrCode = 0;

	int result = WSAStartup(MAKEWORD(2, 2), &WsaData);
	if (result < 0)
	{
		ErrCode = WSAGetLastError();
		printf("Failed to start WSA. Error: %d", ErrCode);
		return KReturn::FAILED;
	}

	//Created socket
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket < 0)
	{
		ErrCode = WSAGetLastError();
		printf("Failed to create listener socket. Error: %d", ErrCode);
		return KReturn::FAILED;
	}

	//Binding socket to port
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	result = bind(ListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if(result < 0)
	{
		ErrCode = WSAGetLastError();
		printf("Failed to bind socket to port. Error: %d", ErrCode);
		return KReturn::FAILED;
	}

	//Awaiting connections
	result = listen(ListenSocket, 0);
	if (result < 0)
	{
		ErrCode = WSAGetLastError();
		printf("Socket Error. Error: %d", ErrCode);
		closesocket(ListenSocket);
		WSACleanup();
		return KReturn::FAILED;
	}
	
	printf("Server listening on port: %d \n", port);

	//Setting up sets

	//Clears the bitset
	FD_ZERO(&MasterSet);
	//adds the listening socket to the master_set. Select will now watch this socket
	FD_SET(ListenSocket, &MasterSet);
	//Starts tracking the highest socket id
	MaxSocketNum = ListenSocket;

	while (true)
	{
		//Copy master
		ReadySet = MasterSet;

		//Wait for activity
		int activity = select(MaxSocketNum + 1, &ReadySet, NULL, NULL, NULL);


		//Loop through all the ready sockets
		for (int i = 0; i <= ReadySet.fd_count; i++)
		{
			//The ready socket
			SOCKET s = ReadySet.fd_array[i];
			
			if (s == ListenSocket)
			{
				//accept the connection
				SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
					
				if (ClientSocket < 0)
				{
					ErrCode = WSAGetLastError();
					printf("accept Failed Error: %d\n", ErrCode);
					closesocket(ClientSocket);
					WSACleanup();
					return KReturn::FAILED;
				}
					
				if (MasterSet.fd_count > ChatCap)
				{
					send_packet(ClientSocket, "Server is at capacity. Try again later.\n");
					shutdown(ClientSocket, SD_BOTH);
					closesocket(ClientSocket);
					continue;
				}


				//add client socket to the master set
				FD_SET(ClientSocket, &MasterSet);

				send_packet(ClientSocket,
					"Welcome to KOS COMS\n"
					"Commands start with ~\n"
					"Try ~help to get started!\n");

				send_packet(ClientSocket, "Tip: ~register <username> <password> then ~login <username> <password>\n");

			}
			//Incoming data
			else
			{
				
				char buffer[512];
				int payload_size = recv_packet(s, buffer);

				if (payload_size <= 0)
				{
					//close socket
					closesocket(s);
					//remove from master set
					FD_CLR(s, &MasterSet);
				}
				else
				{
					server_commands(s, buffer);
				}
			}

			
		}

	}

	return KReturn::SUCCESS;
}

int Server::send_all(SOCKET s, const char* msg, int len)
{
	int bytesSent = 0;

	//While the full msg hasnt been sent
	while (bytesSent < len)
	{
		//Send one byte pre iteration
		int result = send(s, msg + bytesSent, len - bytesSent, 0);

		//Error
		if (result < 0)
		{
			return result;
		}

		//Advance to the next byte
		bytesSent += result;
	}
	//return the lenght of the msg
	return bytesSent;
}

int Server::send_packet(SOCKET s, const char* msg)
{
	//If there is no msg return error
	if (!msg) return SOCKET_ERROR;

	//Get the len of the message
	size_t msg_len = strlen(msg);

	//If msg lenght is bigger than 255
	if (msg_len > 255)
	{
		printf("Msg is bigger than 255 bytes");
	}

	unsigned char len_bytes = (unsigned char)msg_len;

	//Send the lenght of the message over
	if (send_all(s, (const char*)&len_bytes, 1) == SOCKET_ERROR)
		return SOCKET_ERROR;

	//If there is a payload send that one byte at a time
	if (msg_len > 0)
	{
		if (send_all(s, msg, (int)msg_len) == SOCKET_ERROR)
			return SOCKET_ERROR;
	}

	return (int)(1 + msg_len);
}

int Server::revc_exact(SOCKET s, char* incomingMsg, int len)
{
	int incomingBytes = 0;

	//While the full msg hasnt been sent
	while (incomingBytes < len)
	{
		//Send one byte pre iteration
		int result = recv(s, incomingMsg + incomingBytes, len - incomingBytes, 0);

		//client disconnect
		if (result == 0)
		{
			return 0;
		}

		//Error
		if (result == SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}

		//Advance to the next byte
		incomingBytes += result;
	}
	//return the lenght of the msg 
	return incomingBytes;
}

int Server::recv_packet(SOCKET s, char incomingMsg[256])
{
	unsigned char len_byte = 0;

	//Get the message lenght
	int result = revc_exact(s, (char*)&len_byte, 1);
	if (result == 0) return 0; //disconnected
	if (result == SOCKET_ERROR) return SOCKET_ERROR;

	int payload_len = (int)len_byte;

	//Read in the message
	if (payload_len)
	{
		result = revc_exact(s, incomingMsg, payload_len);
		if (result == 0) return 0;
		if (result == SOCKET_ERROR) return SOCKET_ERROR;
	}

	//Adds a null terminate to the end of the string
	incomingMsg[payload_len] = '\0';
	return payload_len;
}

KReturn Server::server_commands(SOCKET s, const char* buffer)
{		
	if (!buffer) return KReturn::UNKNOWN;

	//Data from the commandInput on the client side
	std::string CommandInput(buffer);

	if (CommandInput.empty() || CommandInput[0] != '~')
	{
		if (!LoggedIn(s))
		{
			send_packet(s, "You must ~login before sending messages");
			return KReturn::UNKNOWN;
		}
		
	}

	switch (CommandFromToken(buffer))
	{
		case ServerCommands::help:
		{
			send_packet(s,
				"Commands:\n"
				"  ~help\n"
				"  ~register <username> <password>\n"
				"  ~login <username> <password>\n"
				"  ~logout\n"
				"  ~getlist\n"
				"  ~getlog\n"
				"  ~send <username> <message>\n");
			return KReturn::SUCCESS;
		}
		case ServerCommands::signup:
		{
			//User is logged in
			if (LoggedIn(s))
			{
				send_packet(s, "User is already login");
				return KReturn::UNKNOWN;
			}
			
			//Read data from the CommandInput
			std::istringstream StringStream(CommandInput);
			std::string Command, Username, Password;
			//Read in the data as it was passed in
			StringStream >> Command >> Username >> Password;

			//Check if only ~register was recviced
			if (Username.empty() || Password.empty())
			{
				send_packet(s, "Enter: username password");
				char CommandBuffer[256]{};
				int InputRecv = recv_packet(s, CommandBuffer);
				
				//If nothing recviced cancel command
				if (InputRecv <= 1)
				{
					send_packet(s, "Registration cancelled");
					return KReturn::UNKNOWN;
				}

				//Turn CommandBuffer into a stream and read in the username and password
				std::istringstream StringStream2{ std::string(CommandBuffer) };
				StringStream2 >> Username >> Password;

				if (Username.empty() || Password.empty())
				{
					//Display usage if command
					send_packet(s, "Usage: ~register username password");
					return KReturn::UNKNOWN;
				}
			}

			if (HashTableFind(Username.c_str()) != NULL)
			{
				send_packet(s, "Username already registered");
				return KReturn::UNKNOWN;
			}

			ClientInfo* info = new ClientInfo();
			
			//Set info memory to 0
			memset(info, 0, sizeof(ClientInfo));
			strcpy_s(info->username, MAX_USERNAME, Username.c_str());
			strcpy_s(info->password, MAX_USERNAME, Password.c_str());

			if (!HashTableInsert(info))
			{
				delete info;
				send_packet(s, "Registration failed (username exists or table full).");
				return KReturn::UNKNOWN;
			}

			send_packet(s, "Registration successful now ~login");
			return KReturn::SUCCESS;
		}
		case ServerCommands::login:
		{
			if (LoggedIn(s))
			{
				send_packet(s, "User already logged in.");
				return KReturn::UNKNOWN;
			}

			std::istringstream StringStream(CommandInput);
			std::string cmd, username, password;
			StringStream >> cmd >> username >> password;

			if (username.empty() || password.empty())
			{
				send_packet(s,"Usage: ~login username password");
				return KReturn::SUCCESS;
			}

			ClientInfo* info = HashTableFind(username.c_str());

			if (info == NULL)
			{
				send_packet(s, "User not found, please use ~register command.");
				return KReturn::UNKNOWN;
			}

			if (strcmp(info->password, password.c_str()) != 0)
			{
				send_packet(s, "Incorrect password.");
				return KReturn::UNKNOWN;
			}
			
			SocketToUser[s] = username;
			UserToSocket[username] = s;
			send_packet(s, "Login successful. Welcome to KOS COMS");
			return KReturn::SUCCESS;
		}
	}
	return KReturn::UNKNOWN;
}

