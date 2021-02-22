#define CINTERFACE

#include <iostream>
#include <enet/enet.h>
#include <map>
#include <string>
#include <vector>

#include "ini.h"
#include "squirrel.h"



#pragma warning(disable: 4018)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)
#pragma warning(disable: 8051)
// LINUX: Link to libenet.a [ g++ LUServer.cpp -lstdc++ -L/home/ubuntu/lu_serv -lenet -o Server ]
#ifdef WIN32
#pragma comment(lib,"enet.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib") 
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

	inipp::extract(ini.sections["config"]["server_name"], server_name);
	inipp::extract(ini.sections["config"]["max_players"], max_players);
	inipp::extract(ini.sections["config"]["port"], port);

	printf("Liberty Unleashed 0.1 Server Started\n\n");

	if (server_name.length() == 0) server_name = "Default Server";

	printf("Server name: %s\n", server_name.c_str());
	printf("Max players: %i\n",max_players);
	printf("Port: %i\n", port);

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
 