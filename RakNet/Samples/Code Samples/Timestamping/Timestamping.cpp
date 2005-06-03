#include "RakClientInterface.h"
#include "RakServerInterface.h"
#include "RakNetworkFactory.h"
#include "BitStream.h"
#include "PacketEnumerations.h"
#include "GetTime.h"
using namespace RakNet;

#include <stdio.h>
#include <conio.h>
#include <memory.h>


int main(void)
{
	char serverIP[30];

	RakClientInterface *rakClient=RakNetworkFactory::GetRakClientInterface();
	RakServerInterface *rakServer=RakNetworkFactory::GetRakServerInterface();

	char ch;
	bool isServer;
	printf("Hit 'c' to run as a client.  Hit 's' to run as a server. Hit 'q' to quit\n");

	while (1)
	{
		ch = getch();

		if (ch=='c')
		{
			// Run as a client.  If you don't have another machine, just run 2 instances of this program and use "127.0.0.1"
			puts ("Enter server IP\n");
			scanf("%29s", serverIP);
			rakClient->Connect(serverIP, 2000, 2100, 0, 30);
			printf("Connecting client\n");
			isServer=false;
			break;
		}
		else if (ch=='s')
		{
			// Run as a server.
			rakServer->Start(32, 0, 30, 2000);
			printf("Server started\n");
			isServer=true;
			break;
		}
		else if (ch=='q')
			return 0;
		else
		{
			printf("Bad input.  Enter 'c' 's' or 'q'.\n");
		}
	}

	printf("Entering main loop.  Press 'q' to quit\n'c' to send from the client.\n's' to send from the server.\n");
	Packet *packet;
	unsigned long time;
	ch=0;
	while (1)
	{
		if (kbhit())
			ch=getch();

		if (ch=='q')
			break;

		if (ch=='c' && rakClient->IsConnected())
		{
			BitStream bitStream;

			// When writing a timestamp, the first byte is ID_TIMESTAMP
			// The next 4 bytes is the timestamp itself.

			bitStream.Write((unsigned char)ID_TIMESTAMP);
			
			time=RakNetGetTime();
			bitStream.Write(time);
			rakClient->Send(&bitStream, HIGH_PRIORITY, RELIABLE, 0);
			printf("Sending packet from client at time %i\n", time);
		}
		else if (ch=='s' && rakServer->IsActive())
		{
			BitStream bitStream;
			bitStream.Write((unsigned char)ID_TIMESTAMP);

			time=RakNetGetTime();
			bitStream.Write(time);
			rakServer->Send(&bitStream, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true);
			printf("Sending packet from server at time %i\n", time);
		}

        if (isServer==false)
			packet=rakClient->Receive();
		else
			packet=rakServer->Receive();
		
		if (packet && packet->data[0]==ID_TIMESTAMP)
		{
			// Write the 4 bytes after the first to a variable.  That is the time the packet was sent.
			memcpy((char*)&time, packet->data+1, sizeof(unsigned long));
			printf("Time difference is %i\n", RakNetGetTime() - time);
		}

		if (packet)
			rakServer->DeallocatePacket(packet);

		ch=0;
	}

	// Shutdown stuff.  It's ok to call disconnect on the server if we are a client and vice-versa
	rakServer->Disconnect(0);
	rakClient->Disconnect(0);

	RakNetworkFactory::DestroyRakClientInterface(rakClient);
	RakNetworkFactory::DestroyRakServerInterface(rakServer);

	return 0;
}