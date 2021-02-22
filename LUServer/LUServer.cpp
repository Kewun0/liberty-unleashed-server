#define CINTERFACE

#include <iostream>
#include <enet/enet.h>
#include <map>
#include <string>
#include <vector>

#include "ini.h"
#include "squirrel.h"
#include "sqstdaux.h"
#include "sqstdblob.h"
#include "sqstdio.h"
#include "sqstdmath.h"
#include "sqstdmath.h"
#include "sqstdstring.h"
#include "sqstdsystem.h"
#include "Raknet/MessageIdentifiers.h"
#include "Raknet/RakPeerInterface.h"
#include "Raknet/RakNetStatistics.h"
#include "Raknet/RakNetTypes.h"
#include "Raknet/BitStream.h"
#include "Raknet/RakSleep.h"
#include "Raknet/PacketLogger.h"


#pragma warning(disable: 4018)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)
#pragma warning(disable: 8051)
// LINUX: Link to libenet.a [ g++ LUServer.cpp -lstdc++ -L/home/ubuntu/lu_serv -lenet -o Server ]
#ifdef WIN32
#pragma comment(lib,"enet.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib") 
#pragma comment(lib,"squirrel.lib")
#pragma comment(lib,"sqstdlib.lib")
#endif

ENetEvent event;
ENetAddress address;
ENetHost* server;

class Clients
{
private:
	int m_id;
	int m_health;
	int m_armour;
	std::string m_username;

public:
	Clients(int id) : m_id(id) {}

	void SetHealth(int hp) { m_health = hp; }
	void SetUsername(std::string username) { m_username = username; }

	int GetID() { return m_id; }
	std::string GetUsername() { return m_username; }
};

std::map<int, Clients*> client_map;


void RegisterFunction(HSQUIRRELVM pVM, char* szFunc, SQFUNCTION func, int params, const char* szTemplate)
{
	sq_pushroottable(pVM);

	sq_pushstring(pVM, (const SQChar*)szFunc, -1);
	sq_newclosure(pVM, func, 0);
	if (params != -1)
	{
		sq_setparamscheck(pVM, params, (const SQChar*)szTemplate);
	}
	sq_createslot(pVM, -3);
}

void printfunc(SQVM* pVM, const char* szFormat, ...)
{
	va_list vl;
	char szBuffer[512];
	va_start(vl, szFormat);
	vsprintf(szBuffer, szFormat, vl);
	va_end(vl);
	printf(szBuffer);
}

SQInteger sq_getPlayerName(SQVM* pVM)
{
	SQInteger playerSystemAddress;
	sq_getinteger(pVM, -1, &playerSystemAddress);
	//if (client_map[playerSystemAddress]->GetUsername().length() != 0)
//	{
		
		const char* pName = client_map[playerSystemAddress]->GetUsername().c_str();

		printf("TESTOVIRON%s\n",client_map[playerSystemAddress]->GetUsername().c_str());

		sq_pushstring(pVM, pName, -1);
		return 1;
	//}

	//sq_pushbool(pVM, false);
//	return 1;
}

int sq_register_natives(SQVM* pVM)
{
	//RegisterFunction(pVM, (char*)"MessagePlayer", sq_sendPlayerMessage, 3, ".si");
	//RegisterFunction(pVM, (char*)"GetPlayerHealth", sq_getPlayerHealth, 2, ".i");
	//RegisterFunction(pVM, (char*)"GetPlayerArmour", sq_getPlayerArmour, 2, ".i");
	RegisterFunction(pVM, (char*)"GetPlayerName", (SQFUNCTION)sq_getPlayerName, 2, ".n");

	return 1;
}

class CScript
{
private:
	SQVM* m_pVM;
	char m_szScriptName[256];
	char m_szScriptAuthor[256];
	char m_szScriptVersion[256];
public:

	SQVM* GetVM() { return m_pVM; };

	char* GetScriptName() { return (char*)&m_szScriptName; };
	char* GetScriptAuthor() { return (char*)&m_szScriptAuthor; };
	char* GetScriptVersion() { return (char*)&m_szScriptVersion; };
	void SetScriptAuthor(const char* szAuthor) { strncpy(m_szScriptAuthor, szAuthor, sizeof(m_szScriptAuthor)); };
	void SetScriptVersion(const char* szVersion) { strncpy(m_szScriptVersion, szVersion, sizeof(m_szScriptVersion)); };
	CScript(const char* szScriptName)
	{
		char szScriptPath[512];
		sprintf(szScriptPath, "%s", szScriptName);

		FILE* fp = fopen(szScriptPath, "rb");

		if (!fp) {
			printf("Script: Failed to load script %s - file does not exist \n", szScriptName);
			return;
		}

		fclose(fp);

		strcpy(m_szScriptName, szScriptName);

		m_pVM = sq_open(1024);

		SQVM* pVM = m_pVM;

		sqstd_seterrorhandlers(pVM);

		sq_setprintfunc(pVM, (SQPRINTFUNCTION)printfunc, (SQPRINTFUNCTION)printfunc);

		sq_pushroottable(pVM);

		sqstd_register_bloblib(pVM);	

		sqstd_register_iolib(pVM);

		sqstd_register_mathlib(pVM);

		sqstd_register_stringlib(pVM);

		sqstd_register_systemlib(pVM);

		sq_register_natives(pVM);

		if (SQ_FAILED(sqstd_dofile(pVM, (const SQChar*)szScriptPath, SQFalse, SQTrue)))
		{
			printf("Script: Compiling script error. Halting \n");
			return;
		}

		sq_pop(pVM, 1);

		printf("Script: Loaded %s \n", szScriptName);

		return;
	}
};
CScript* test;

void onScriptLoad()
{
	if (test)
	{
		SQVM* pVM = test->GetVM();
		int iTop = sq_gettop(pVM);
		sq_pushroottable(pVM);
		sq_pushstring(pVM, (const SQChar*)"onScriptLoad", -1);
		if (SQ_SUCCEEDED(sq_get(pVM, -2)))
		{
			sq_pushroottable(pVM);
			sq_call(pVM, 1, true, true);
		}
		sq_settop(pVM, iTop);
	}
}

void onPlayerJoin(int playerId)
{
	if (test)
	{
		SQVM* pVM = test->GetVM();
		int iTop = sq_gettop(pVM);
		sq_pushroottable(pVM);
		sq_pushstring(pVM, (const SQChar*)"onPlayerJoin", -1);
		if (SQ_SUCCEEDED(sq_get(pVM, -2)))
		{
			sq_pushroottable(pVM);
			sq_pushinteger(pVM, playerId);
			sq_call(pVM, 2, true, true);
		}
		sq_settop(pVM, iTop);
	}
}

void onPlayerConnect(int playerId)
{
	if (test)
	{
		SQVM* pVM = test->GetVM();
		int iTop = sq_gettop(pVM);
		sq_pushroottable(pVM);
		sq_pushstring(pVM, (const SQChar*)"onPlayerConnect", -1);
		if (SQ_SUCCEEDED(sq_get(pVM, -2)))
		{
			sq_pushroottable(pVM);
			sq_pushinteger(pVM, playerId);
			sq_call(pVM, 2, true, true);
		}
		sq_settop(pVM, iTop);
	}
}


void onPlayerPart(int playerId)
{
	if (test)
	{
		SQVM* pVM = test->GetVM();
		int iTop = sq_gettop(pVM);
		sq_pushroottable(pVM);
		sq_pushstring(pVM, (const SQChar*)"onPlayerPart", -1);
		if (SQ_SUCCEEDED(sq_get(pVM, -2)))
		{
			sq_pushroottable(pVM);
			sq_pushinteger(pVM, playerId);
			sq_call(pVM, 2, true, true);
		}
		sq_settop(pVM, iTop);
	}
}


void SendPacket(ENetPeer* peer, const char* data)
{
	ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
	enet_peer_send(peer, 0, packet);
}

void BroadcastPacket(ENetHost* server, const char* data)
{
	ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
	enet_host_broadcast(server, 0, packet);
}

void DecryptPacket(char* data, int client_id)
{
	std::string str(data);
	std::string buf;
	std::stringstream ss(str);
	std::vector<std::string> tokens;

	while (ss >> buf)
	{
		tokens.push_back(buf);
	}

	if (tokens[0] == "8|")
	{
		float health, x, y, z;
		health = atoi(tokens[1].c_str());

		x = atof(tokens[2].c_str());
		y = atof(tokens[3].c_str());
		z = atof(tokens[4].c_str());
	}

	if (data[0] == '9' && data[1] == '|')
	{
		char* token = strtok(data, "|");
		std::string packet;
		while (token != NULL)
		{
			packet = token;
			token = strtok(NULL, "|");
		}
		char data[256];
		sprintf(data, "9| %s:%s", client_map[client_id]->GetUsername().c_str(), packet.c_str());
		BroadcastPacket(server, data);
	}
}

void ParseData(ENetHost* server, int id, char* data)
{
	int data_type;
	sscanf(data, "%d|", &data_type);

	

	switch (data_type)
	{
		case 1:
		{
			char msg[80];
			sscanf(data, "%*d|%[^\n]", &msg);

			char send_data[1024] = { '\0' };
			sprintf(send_data, "1|%d|%s", id, msg);
			BroadcastPacket(server, send_data);
			break;
		}
		case 2:
		{
			char username[80];
			sscanf(data, "2|%[^\n]", &username);
			
			char send_data[1024] = { '\0' };
			sprintf(send_data, "2|%d|%s", id, username);
			onPlayerJoin(id);
			printf("%s has joined the server\n", username);

			BroadcastPacket(server, send_data);
			client_map[id]->SetUsername(username);

			break;
		}
		case 9:
		{
			DecryptPacket(data, id);
			break;
		}
		case 8:
		{
			DecryptPacket(data, id);
			BroadcastPacket(server, data);
			break;
		}
	}
}
#include <fstream>
std::string server_name;
int max_players = 32;
int port = 7777;

inline bool does_file_exist(const std::string& name)
{
	#ifdef WIN32
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	#else
		return (access(name.c_str(), F_OK) != -1);
	#endif
}

void ServerThread()
{

}

int main(int argc, char** argv)
{
	if (!does_file_exist("server.ini"))
	{
		printf( "ERROR: server.ini is missing. Using default configuration\n\n");
	}
	inipp::Ini<char> ini;
	std::ifstream is("server.ini");
	ini.parse(is);
	server_name = "Default Server";

	std::string script_name = "";

	inipp::extract(ini.sections["config"]["server_name"], server_name);
	inipp::extract(ini.sections["config"]["script"], script_name);
	inipp::extract(ini.sections["config"]["max_players"], max_players);
	inipp::extract(ini.sections["config"]["port"], port);

	printf("Liberty Unleashed 0.1 Server Started\n\n");

	if (server_name.length() == 0) server_name = "Default Server";

	printf("Server name: %s\n", server_name.c_str());
	printf("Max players: %i\n",max_players);
	printf("Port: %i\n", port);

	if (script_name.length() == 0) {
		printf("Script: No script specified\n");
	}
	else { test = new CScript(script_name.c_str()); }

	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		return EXIT_FAILURE;
	}

	atexit(enet_deinitialize);

	address.host = ENET_HOST_ANY; 
	
	address.port = port;
	server = enet_host_create(&address,max_players,1,0,0);

	if (server == NULL)
	{
		printf("An error occurred while trying to create an ENet server host.");
		return 1;
	}
	int new_player_id = 0;
	
	while (1!=2)
	{
		ENetEvent event;
		
		while (enet_host_service(server, &event, 0) > 0)
		{ 
			if (event.type == ENET_EVENT_TYPE_CONNECT)
			{
				for (auto const& x : client_map)
				{
					char send_data[1024] = { '\0' };
					sprintf(send_data, "2|%d|%s", x.first, x.second->GetUsername().c_str());
					BroadcastPacket(server, send_data);
				}

				new_player_id++;
				client_map[new_player_id] = new Clients(new_player_id);
				event.peer->data = client_map[new_player_id];

				char data_to_send[126] = { '\0' };
				sprintf(data_to_send, "3|%d", new_player_id);
				SendPacket(event.peer, data_to_send);
				onPlayerConnect(new_player_id);
			}
			else if (event.type == ENET_EVENT_TYPE_RECEIVE)
			{
				char data[256];
				sprintf(data, "%s", event.packet->data);
				ParseData(server, static_cast<Clients*>(event.peer->data)->GetID(), data);
				enet_packet_destroy(event.packet);
			}
			else if (event.type== ENET_EVENT_TYPE_DISCONNECT)
			{
				onPlayerPart(static_cast<Clients*>(event.peer->data)->GetID());
				printf("%s has left the server\n", static_cast<Clients*>(event.peer->data)->GetUsername().c_str());
				char disconnected_data[126] = { '\0' };
				sprintf(disconnected_data, "4|%d", static_cast<Clients*>(event.peer->data)->GetID());
				BroadcastPacket(server, disconnected_data);
				event.peer->data = NULL;
			}
		}
	}

	enet_host_destroy(server);

	return 0;
}
 