#include "stdint.h"
#include "stdio.h"
#include <cstdlib>
#include <string>
#include <cstring>


#define MAX_USERNAME 256
#define MAX_PASSWORD 256
#define TABLE_SIZE 10


struct ClientInfo
{
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
};

static	ClientInfo* HashTable[TABLE_SIZE];

//Create the hash table
inline void InitHashTable()
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		HashTable[i] = NULL;
	}
}

//Places the username within the hash table
inline unsigned int hash(const char* username)
{
	int length = static_cast<int>(strnlen(username, MAX_USERNAME));
	unsigned int hash_value = 0;
	for (int i = 0; i < length; i++)
	{
		hash_value += username[i];
		hash_value = (hash_value * username[i]) % TABLE_SIZE;
	}
	return hash_value;
}

inline void PrintTable()
{
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		if (HashTable[i] == NULL)
		{
			printf("\t%i\t---\n", i);
		}
		else
		{
			printf("\t%i\t%s\n", i, HashTable[i]->username);
		}
	}
}


inline bool HashTableInsert(ClientInfo* info)
{
	if (info == NULL) return false;
	unsigned int start = hash(info->username);

	for (unsigned int step = 0; step < TABLE_SIZE; step++)
	{
		unsigned int index = (start + step) % TABLE_SIZE;
		if (HashTable[index] == NULL)
		{
			HashTable[index] = info;
			return true;
		}
		// Username already exists
		if (strncmp(HashTable[index]->username, info->username, MAX_USERNAME) == 0)
		{
			return false;
		}
	}
	return false;
}

inline ClientInfo* HashTableFind(const char* username)
{
	if (!username) return NULL;
	unsigned int start = hash(username);

	for (unsigned int step = 0; step < TABLE_SIZE; step++)
	{
		unsigned int index = (start + step) % TABLE_SIZE;
		if (HashTable[index] == NULL)
		{
			// Hit an empty slot => not in the table.
			return NULL;
		}
		if (strncmp(HashTable[index]->username, username, MAX_USERNAME) == 0)
		{
			return HashTable[index];
		}
	}
	return NULL;
}