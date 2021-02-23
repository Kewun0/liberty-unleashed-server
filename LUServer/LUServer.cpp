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
 
#include "raknet/MessageIdentifiers.h"
#include "raknet/peerinterface.h"
#include "raknet/statistics.h"
#include "raknet/types.h"
#include "raknet/BitStream.h"
#include "raknet/sleep.h"
#include "raknet/PacketLogger.h"


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
#pragma comment(lib,"slikenet.lib")

#endif

ENetEvent event;
ENetAddress address;
ENetHost* server;

class CPlayer
{
public:
	int m_ID = -1;
	int m_skinID;
	int m_weapon;

	bool m_bActive = false;

	float m_fHealth;
	float m_fArmour;

	float m_fPosX;
	float m_fPosY;
	float m_fPosZ;

	float m_fHeading;

	std::string m_systemAddress;
	std::string m_LUID;

	CPlayer()
	{
		m_ID = -1;
		m_bActive = false;
	}
};
CPlayer* Players[256];

int GetFreePlayerSlot()
{
	for (int i = 0; i <= 255; i++)
	{
		if (Players[i]->m_bActive == false)
		{
			return i;
		}
	}
	return -1;
}

void RemovePlayer(int playerID)
{
	Players[playerID]->m_bActive = false;
	Players[playerID]->m_ID = -1;
}



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

unsigned char GetPacketIdentifier(SLNet::Packet* p)
{
	if (p == 0)
		return 255; 

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		RakAssert(p->length > sizeof(SLNet::MessageID) + sizeof(SLNet::Time));
		return (unsigned char)p->data[sizeof(SLNet::MessageID) + sizeof(SLNet::Time)];
	}
	else
		return (unsigned char)p->data[0];
}

void ProcessPacket(int playerID, unsigned char* data)
{
	if (data[0] == 'L' && data[1] == 'U' && data[2] == 'I' && data[3] == 'D')
	{
		unsigned char* _luid = data + 4;
		printf("\n[PACKET_TYPE_LUID] Received LUID From %i, %s", playerID,_luid);
	}
	if (data[0] == 'N' && data[1] == 'A' && data[2] == 'M' && data[3] == 'E')
	{
		unsigned char* _nick = data + 4;
		printf("\n[PACKET_TYPE_NAME] Received Nickname From %i, %s", playerID,_nick);
	}
}

int main(int argc, char** argv)
{
	for (int i = 0; i <= 255; i++)
	{
		Players[i] = new CPlayer();
	}
	if (!does_file_exist("server.ini"))
	{
		printf("ERROR: server.ini is missing. Using default configuration\n\n");
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
	printf("Max players: %i\n", max_players);
	printf("Port: %i\n", port);

	if (script_name.length() == 0) {
		printf("Script: No script specified\n");
	}
	else { test = new CScript(script_name.c_str()); }
	 

	SLNet::RakPeerInterface* server = SLNet::RakPeerInterface::GetInstance();
	SLNet::RakNetStatistics* rss;
	server->SetTimeoutTime(30000, SLNet::UNASSIGNED_SYSTEM_ADDRESS);

	SLNet::Packet* p;

	unsigned char packetIdentifier;   

	SLNet::SystemAddress clientID = SLNet::UNASSIGNED_SYSTEM_ADDRESS;

	char portstring[30];

	SLNet::SocketDescriptor socketDescriptors[2];
	socketDescriptors[0].port = static_cast<unsigned short>(port);
	socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
	socketDescriptors[1].port = static_cast<unsigned short>(port);
	socketDescriptors[1].socketFamily = AF_INET6; // Test out IPV6
	bool b = server->Startup(max_players, socketDescriptors, 2) == SLNet::RAKNET_STARTED;
	server->SetMaximumIncomingConnections(max_players);

	if (!b)
	{
		b = server->Startup(4, socketDescriptors, 1) == SLNet::RAKNET_STARTED;
		if (!b)
		{
			puts("Server failed to start: Port in use?");
			exit(1);
		}
	}

	server->SetOccasionalPing(true);
	server->SetUnreliableTimeout(1000);

	DataStructures::List< SLNet::RakNetSocket2* > sockets;
	server->GetSockets(sockets);


	for (unsigned int i = 0; i < server->GetNumberOfAddresses(); i++)
	{
		SLNet::SystemAddress sa = server->GetInternalID(SLNet::UNASSIGNED_SYSTEM_ADDRESS, i);
	}
	char message[2048];

	for (;;)
	{ 
		RakSleep(30);
		for (p = server->Receive(); p; server->DeallocatePacket(p), p = server->Receive())
		{
			packetIdentifier = GetPacketIdentifier(p);
			
			switch (packetIdentifier)
			{
			case ID_DISCONNECTION_NOTIFICATION:
				printf("ID_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));;
				break;
			case ID_NEW_INCOMING_CONNECTION:
				printf("ID_NEW_INCOMING_CONNECTION from %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
				clientID = p->systemAddress;
				printf("clientID: %i\n", clientID.systemIndex);
				//printf("Remote internal IDs:\n");
				for (int index = 0; index < MAXIMUM_NUMBER_OF_INTERNAL_IDS; index++)
				{
					SLNet::SystemAddress internalId = server->GetInternalID(p->systemAddress, index);
					if (internalId != SLNet::UNASSIGNED_SYSTEM_ADDRESS)
					{
					//	printf("%i. %s\n", index + 1, internalId.ToString(true));
					}
				}

				break;

			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
				break;

			case ID_CONNECTED_PING:
			case ID_UNCONNECTED_PING:
				printf("Ping from %s\n", p->systemAddress.ToString(true));
				break;

			case ID_CONNECTION_LOST:
				printf("ID_CONNECTION_LOST from %s\n", p->systemAddress.ToString(true));;
				break;

			default:
				//printf("%s\n", p->data);
				
				//printf("[UNKNOWN PACKET] %s\n", p->data);
				//if (p->data[0] == '1') printf("Maybe LUID? \n");

				clientID = p->systemAddress;
				//printf("clientID: %i\n", clientID.systemIndex);

				ProcessPacket(clientID.systemIndex,p->data);

				sprintf(message, "%s", p->data);
				server->Send(message, (const int)strlen(message) + 1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, true);
				
				break;
			}

		}
	}

	return 0;
}