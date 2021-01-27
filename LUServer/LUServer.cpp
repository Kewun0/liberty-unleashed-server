#define CINTERFACE

#include <iostream>
#include <enet/enet.h>
#include <map>


#pragma comment(lib,"enet.lib")

#pragma warning(disable: 4018)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib") 

class Clients
{
private:
	int m_id;
	std::string m_username;

public:
	Clients(int id) : m_id(id) {}

	void SetUsername(std::string username) { m_username = username; }

	int GetID() { return m_id; }
	std::string GetUsername() { return m_username; }
};

std::map<int, Clients*> client_map;

void SendPacket(ENetPeer* peer, const char* data)
{
	ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet);
}

void BroadcastPacket(ENetHost* server, const char* data)
{
	ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(server, 0, packet);
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

			BroadcastPacket(server, send_data);
			client_map[id]->SetUsername(username);

			break;
		} 
	}
}

int main(int argc, char** argv)
{
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		return EXIT_FAILURE;
	}
	atexit(enet_deinitialize);

	ENetEvent event;
	ENetAddress address;
	ENetHost* server;

	address.host = ENET_HOST_ANY; 
	
	address.port = 7777;



	server = enet_host_create(&address	/* the address to bind the server host to */,
		32	/* allow up to 32 clients and/or outgoing connections */,
		1	/* allow up to 1 channel to be used, 0. */,
		0	/* assume any amount of incoming bandwidth */,
		0	/* assume any amount of outgoing bandwidth */);

	if (server == NULL)
	{
		printf("An error occurred while trying to create an ENet server host.");
		return 1;
	}
	int new_player_id = 0;
	// gameloop
	while (1!=2)
	{
		ENetEvent event;
		/* Wait up to 1000 milliseconds for an event. */
		
		while (enet_host_service(server, &event, 1000) > 0)
		{ 
			if (event.type == ENET_EVENT_TYPE_CONNECT)
			{
				printf("A new client connected from %x:%u.\n",
					event.peer->address.host,
					event.peer->address.port);

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
				printf("A packet of length %u containing %s was received from %s on channel %u.\n",
					event.packet->dataLength,
					event.packet->data,
					event.peer->data,
					event.channelID);
				char data[256];
				sprintf(data, "%s", event.packet->data);
				ParseData(server, static_cast<Clients*>(event.peer->data)->GetID(), data);
				enet_packet_destroy(event.packet);
			}
			else if (event.type== ENET_EVENT_TYPE_DISCONNECT){
				printf("%s disconnected.\n", event.peer->data);
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
 