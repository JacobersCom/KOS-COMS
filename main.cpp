#include "Server.h"

int main()
{

	int port;
	int chatCap;
	char commadChar = '~';
	Server server;


	printf("KOS COMS\n\n");

	printf("Enter TCP port number: ");
	scanf_s("%d", &port);

	printf("Enter chat capacity (max clients): ");
	scanf_s("%d", &chatCap);

	printf("Command character for the server is ~\n\n");

	printf("Server Configs\n");
	printf("Port: %d\n", port);
	printf("Chat Capacity: %d\n", chatCap);

	printf("Setup complete. Server is booting up...\n");

	server.Init(port, chatCap);
	return 0;
}