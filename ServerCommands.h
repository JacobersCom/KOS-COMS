#pragma once

#include <string>

enum class ServerCommands
{
	help,
	signup,
	login,

	Unknown,
};

inline ServerCommands CommandFromToken(const char* cmd)
{
	//true s is cmd false s is empty string
	std::string token;
	std::string string = cmd;

	if (string != "")
	{
		size_t space = string.find(' ');

		//Token found
		if (space != std::string::npos)
		{
			token = string.substr(0, space);
		}
	}

	if (token == "~help") return ServerCommands::help;
	if (token == "~register") return ServerCommands::signup;
	if (token == "~login") return ServerCommands::login;

	return ServerCommands::Unknown;
}