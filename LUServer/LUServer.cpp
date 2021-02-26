#define CINTERFACE

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>

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

#ifdef WIN32
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib") 
#pragma comment(lib,"squirrel.lib")
#pragma comment(lib,"sqstdlib.lib")
#pragma comment(lib,"slikenet.lib")
#endif

std::string server_name;
int max_players = 32;
int port = 7777;
SLNet::RakPeerInterface* server = SLNet::RakPeerInterface::GetInstance();
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

	
	std::string m_LUID;
	std::string m_Nick;

	SLNet::SystemAddress m_systemAddress;

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
	if (Players[playerSystemAddress]->m_Nick.length() != 0 && Players[playerSystemAddress]->m_bActive == true)
	{
		sq_pushstring(pVM, Players[playerSystemAddress]->m_Nick.c_str(), -1);
	}
	else
		sq_pushbool(pVM, false);

	return 1;
}

SQInteger sq_getPlayerLUID(SQVM* pVM)
{
	SQInteger playerSystemAddress;
	sq_getinteger(pVM, -1, &playerSystemAddress);
	if (Players[playerSystemAddress]->m_LUID.length() != 0 && Players[playerSystemAddress]->m_bActive == true )
	{
		sq_pushstring(pVM, Players[playerSystemAddress]->m_LUID.c_str(), -1);
	}
	else
		sq_pushbool(pVM, false);

	return 1;
}

SQInteger sq_kickPlayer(SQVM* pVM)
{
	SQInteger playerSystemAddress;
	sq_getinteger(pVM, -1, &playerSystemAddress);
	if (Players[playerSystemAddress]->m_bActive == true)
	{
		Players[playerSystemAddress]->m_bActive = false;
		Players[playerSystemAddress]->m_ID = -1;
		server->CloseConnection(Players[playerSystemAddress]->m_systemAddress, true, 0);
	}
	return 1;
}

SQInteger sq_message(SQVM* pVM)
{
	const char* msg;
	sq_getstring(pVM, -1, &msg);

	char toSend[255];
	sprintf(toSend, "MESS%s",msg);
	server->Send(toSend, strlen(toSend)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, SLNet::UNASSIGNED_SYSTEM_ADDRESS, true);

	return 1;
}

SQInteger sq_messagePlayer(SQVM* pVM)
{
	SQInteger playerSystemAddress;
	const char* msg;
	sq_getstring(pVM, -2, &msg);
	sq_getinteger(pVM, -1, &playerSystemAddress);
	char pakFormat[256];
	sprintf(pakFormat, "MESS%s\0", msg);
	server->Send(pakFormat, strlen(pakFormat)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, server->GetSystemAddressFromIndex(playerSystemAddress), false);
	return 1;
}  

SQInteger sq_setFPSLimit(SQVM* pVM)
{
	SQInteger playerSystemAddress;
	SQInteger newFPSValue;
	sq_getinteger(pVM, -2, &playerSystemAddress);
	sq_getinteger(pVM, -1, &newFPSValue);
	char packet[32];
	sprintf(packet, "FPS%i", (int)newFPSValue);
	
	server->Send(packet, strlen(packet), HIGH_PRIORITY, RELIABLE_ORDERED,0, server->GetSystemAddressFromIndex(playerSystemAddress), false);
	return 1;
}

int sq_register_natives(SQVM* pVM)
{
	RegisterFunction(pVM, (char*)"GetPlayerName", (SQFUNCTION)sq_getPlayerName, 2, ".n");
	RegisterFunction(pVM, (char*)"GetPlayerLUID", (SQFUNCTION)sq_getPlayerLUID, 2, ".n");
	RegisterFunction(pVM, (char*)"KickPlayer", (SQFUNCTION)sq_kickPlayer, 2, ".n");
	RegisterFunction(pVM, (char*)"Message", (SQFUNCTION)sq_message, 2, ".s");
	RegisterFunction(pVM, (char*)"MessagePlayer", (SQFUNCTION)sq_messagePlayer, 3, ".sn");
	RegisterFunction(pVM, (char*)"SetFPSLimit", (SQFUNCTION)sq_setFPSLimit, 3, ".nn");
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
	CScript(const char* szScriptName){
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

void onPlayerChat(int playerID, const char* msg)
{
	if (test)
	{
		SQVM* pVM = test->GetVM();
		int iTop = sq_gettop(pVM);
		sq_pushroottable(pVM);
		sq_pushstring(pVM, (const SQChar*)"onPlayerChat", -1);
		if (SQ_SUCCEEDED(sq_get(pVM, -3)))
		{
			sq_pushroottable(pVM);
			sq_pushinteger(pVM, playerID);
			sq_pushstring(pVM, msg,-1);
			sq_call(pVM, 3, true, true);
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

void ProcessPacket(SLNet::SystemAddress Client, int playerID, unsigned char* data)
{
	if (data[0] == 'L' && data[1] == 'U' && data[2] == 'I' && data[3] == 'D')
	{
		unsigned char* _luid = data + 4;
		if (Players[playerID]->m_bActive == false)
		{
			Players[playerID]->m_bActive = true;
			Players[playerID]->m_LUID = (char*)_luid;
			Players[playerID]->m_ID = playerID;
		}
		onPlayerJoin(playerID);
		printf("\n%s has joined the server",  Players[playerID]->m_Nick.c_str());

	}
	if (data[0] == 'N' && data[1] == 'A' && data[2] == 'M' && data[3] == 'E')
	{
		unsigned char* _nick = data + 4;
		Players[playerID]->m_Nick = (char*)_nick;
		onPlayerConnect(playerID);
		Players[playerID]->m_systemAddress = Client;
	}
	if (data[0] == 'M' && data[1] == 'E' && data[2] == 'S' && data[3] == 'S')
	{
		if (Players[playerID]->m_bActive) {
			unsigned char* _chatmsg = data + 4;
			onPlayerChat(playerID, (const char*)_chatmsg);
			char toSend[255];
			sprintf(toSend, "MESS%s: %s", Players[playerID]->m_Nick.c_str(), _chatmsg);
			server->Send(toSend, strlen(toSend)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, SLNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		}
	}
}

void onPlayerDisconnect(int playerID)
{
	printf("\n%s has left the server", Players[playerID]->m_Nick.c_str());
	Players[playerID]->m_bActive = false;
	Players[playerID]->m_ID = -1;
	onPlayerPart(playerID);
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
	 
	printf("\n");
	
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

				onPlayerDisconnect(p->systemAddress.systemIndex);

				//printf("ID_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));;
				break;
			case ID_NEW_INCOMING_CONNECTION:
				//printf("ID_NEW_INCOMING_CONNECTION from %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
				clientID = p->systemAddress;
				//printf("clientID: %i\n", clientID.systemIndex);

				break;

			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
				break;

			case ID_CONNECTED_PING:
			case ID_UNCONNECTED_PING:
				printf("Ping from %s\n", p->systemAddress.ToString(true));
				break;

			case ID_CONNECTION_LOST:

			//	printf("ID_CONNECTION_LOST from %s\n", p->systemAddress.ToString(true));;
				onPlayerDisconnect(p->systemAddress.systemIndex);
				//printf("%s has lost connection to the server\n", Players[p->systemAddress.systemIndex]->m_Nick.c_str());
				break;

			default:

				clientID = p->systemAddress;
				ProcessPacket(clientID,clientID.systemIndex,p->data);
				sprintf(message, "%s", p->data);
				server->Send(message, (const int)strlen(message) + 1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, true);
				break;
			}

		}
	}

	return 0;
}