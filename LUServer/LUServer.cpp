#define CINTERFACE

#include <iostream>
#include <enet/enet.h>

#pragma comment(lib,"enet.lib")

#pragma warning(disable: 4018)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib") 

ENetEvent event;
ENetAddress address;
ENetHost* server;



int main()
{
	printf("Liberty Unleashed 0.1 Server started \n");
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);
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

	while (true)
	{
		ENetEvent event;
		while (enet_host_service(server, &event, 50) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				printf("A new client connected from %x:%u.\n",
					event.peer->address.host,
					event.peer->address.port);
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				printf("A packet of length %u containing %s was received from %s on channel %u.\n",
					event.packet->dataLength,
					event.packet->data,
					event.peer->data,
					event.channelID);
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				printf("%s disconnected.\n", event.peer->data);
				event.peer->data = NULL;
			}
		}
	}

	enet_host_destroy(server);
	return 1;
}
