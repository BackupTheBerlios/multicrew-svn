#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "Rand.h" // randomMT
#include "PacketEnumerations.h" // Enumerations
#include "NetworkTypes.h" // PlayerID
#include <conio.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h> // Sleep
#endif

#ifdef _DEBUG
static const int NUMBER_OF_CLIENTS=10;
#else
static const int NUMBER_OF_CLIENTS=100;
#endif

void ShowHelp(void)
{
	printf("System started.\n(D)isconnect a random client silently\n(C)onnect a random client\n(R)andom silent disconnects and connects for all clients.\n(N)otify server of random disconnections.\nSpace to verify connection list.\n(H)elp\n(Q)uit\n");
}

int main(void)
{
	RakPeerInterface *server;
	RakPeerInterface *clients[NUMBER_OF_CLIENTS];
	unsigned index, connectionCount, unacknowledgedPacketSize;
	unsigned char ch;
	PlayerID serverID;
	Packet *p;
	unsigned short numberOfSystems;
	RakNetStatisticsStruct *rss;
	unsigned packetOrigin;

	// Used to refer to systems.  We already know the IP
	serverID.binaryAddress=inet_addr("127.0.0.1");
	serverID.port=60000;

	printf("Dropped Connection Test.\n");
	

	server=RakNetworkFactory::GetRakPeerInterface();
//	server->InitializeSecurity(0,0,0,0);
	server->Initialize(NUMBER_OF_CLIENTS, 60000, 0);
	server->SetMaximumIncomingConnections(NUMBER_OF_CLIENTS);

	for (index=0; index < NUMBER_OF_CLIENTS; index++)
	{
		clients[index]=RakNetworkFactory::GetRakPeerInterface();
		clients[index]->Initialize(1,60001+index,0);
		clients[index]->Connect("127.0.0.1", 60000, 0, 0);

		#ifdef _WIN32
				Sleep(10);
		#else
				usleep(10 * 1000);
		#endif
		printf("%i. ", index);
	}

	ShowHelp();

	while (1)
	{
		// User input
		if (kbhit())
		{
			ch=getch();

			if (ch=='d' || ch=='D')
			{
				index = randomMT() % NUMBER_OF_CLIENTS;
				
				clients[index]->GetConnectionList(0, &numberOfSystems);
				clients[index]->CloseConnection(serverID, false,0);
				if (numberOfSystems==0)
					printf("Client %i silently closing inactive connection.\n",index);
				else
					printf("Client %i silently closing active connection.\n",index);
			}
			else if (ch=='c' || ch=='C')
			{
				index = randomMT() % NUMBER_OF_CLIENTS;

				clients[index]->GetConnectionList(0, &numberOfSystems);
				clients[index]->Connect("127.0.0.1", 60000, 0, 0);
				if (numberOfSystems==0)
					printf("Client %i connecting to same existing connection.\n",index);
				else
					printf("Client %i connecting to closed connection.\n",index);
			}
			else if (ch=='r' || ch=='R' || ch=='n' || ch=='N')
			{
				printf("Randomly connecting and disconnecting each client\n");
				for (index=0; index < NUMBER_OF_CLIENTS; index++)
				{
					if ((randomMT()%2)==0)
					{
						if (clients[index]->IsActive())
						{
							if (ch=='r' || ch=='R')
								clients[index]->CloseConnection(serverID, false, 0);
							else
								clients[index]->CloseConnection(serverID, true, 100);
						}
					}
					else
					{
						clients[index]->Connect("127.0.0.1", 60000, 0, 0);
					}
				}
			}
			else if (ch==' ')
			{
				server->GetConnectionList(0, &numberOfSystems);
				printf("The server thinks %i clients are connected.\n", numberOfSystems);
				connectionCount=0;
				for (index=0; index < NUMBER_OF_CLIENTS; index++)
				{
					clients[index]->GetConnectionList(0, &numberOfSystems);
					if (numberOfSystems>1)
						printf("Bug: Client %i has %i connections\n", index, numberOfSystems);
					if (numberOfSystems==1)
					{
						connectionCount++;
					}
				}
				printf("%i clients are actually connected.\n", connectionCount);
			}
			else if (ch=='h' || ch=='H')
			{
				ShowHelp();
			}
			else if (ch=='q' || ch=='Q')
			{
				break;
			}
			ch=0;
		}

		// Parse messages
		
		while (1)
		{
			packetOrigin=65535;
			p = server->Receive();
			if (p==0)
			{
				for (index=0; index < NUMBER_OF_CLIENTS; index++)
				{
					p = clients[index]->Receive();
					if (p!=0)
					{
						packetOrigin=index;
						break;						
					}
				}
			}

			if (p)
			{
				switch (p->data[0])
				{
				case ID_CONNECTION_REQUEST_ACCEPTED:
					printf("%i: ID_CONNECTION_REQUEST_ACCEPTED.\n",packetOrigin);
					break;
				case ID_DISCONNECTION_NOTIFICATION:
					// Connection lost normally
					printf("%i: ID_DISCONNECTION_NOTIFICATION.\n",packetOrigin);
					break;

				case ID_NEW_INCOMING_CONNECTION:
					// Somebody connected.  We have their IP now
					printf("%i: ID_NEW_INCOMING_CONNECTION.\n",packetOrigin);
					break;

				case ID_RECEIVED_STATIC_DATA:
					// Got static data
					printf("%i: ID_RECEIVED_STATIC_DATA.\n",packetOrigin);
					break;

				case ID_MODIFIED_PACKET:
					// Cheater!
					printf("%i: ID_MODIFIED_PACKET.\n",packetOrigin);
					break;

				case ID_CONNECTION_LOST:
					// Couldn't deliver a reliable packet - i.e. the other system was abnormally
					// terminated
					printf("%i: ID_CONNECTION_LOST.\n",packetOrigin);
					break;
				default:
					// Ignore anything else
					break;
				}
			}
			else
				break;

			if (packetOrigin==65535)
				server->DeallocatePacket(p);
			else
				clients[packetOrigin]->DeallocatePacket(p);
		}

		// Have everyone send a reliable packet so dropped connections are noticed.
		ch=255;
		server->Send((char*)&ch, 1, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true);

		for (index=0; index < NUMBER_OF_CLIENTS; index++)
			clients[index]->Send((char*)&ch, 1, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true);

		// Sleep so this loop doesn't take up all the CPU time

#ifdef _WIN32
			Sleep(30);
#else
			usleep(30 * 1000);
#endif

	}

	RakNetworkFactory::DestroyRakPeerInterface(server);
	for (index=0; index < NUMBER_OF_CLIENTS; index++)
		RakNetworkFactory::DestroyRakPeerInterface(clients[index]);
	return 1;
}