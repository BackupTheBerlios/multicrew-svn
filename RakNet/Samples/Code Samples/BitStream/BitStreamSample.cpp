// ----------------------------------------------------------------------
// RakNet version 1.411
// BitStreamSample.cpp
// Created by Rakkar Software (rakkar@rakkarsoftware.com) December 8, 2003
// Shows simple use of the bitstream class
// ----------------------------------------------------------------------

#include "Multiplayer.h"
#include "RakServerInterface.h"
#include "RakClientInterface.h"
#include "RakNetworkFactory.h"
#include "BitStream.h"
#include <conio.h> // For kbhit
#include <stdlib.h> // For atoi
#include <string.h> // For strlen

#ifdef _WIN32
#include <windows.h> // Sleep
#else
#include <unistd.h> // usleep
#endif

using namespace RakNet;

// We want to send information about a person and their job.
// We are interested in their name, age, salary, and years employed.
// For this sample we encode the data as follows:
// 1. Number of characters of the person's name
// 2. Person's name (not null terminated)
// 3. Person's age (compressed unsigned char)
// 4. The EmploymentStruct (below)
struct EmploymentStruct
{
	int salary;
	unsigned char yearsEmployed;
};

// You can parse the input data in two ways.
// Either cast input to a struct (such as if you sent a struct)
// Or create a BitStream instance with input as data such as
// BitStream myBitStream(input, (numberOfBitsOfData-1)/8+1);
// where (numberOfBitsOfData-1)/8+1 is the number of bytes of data
// If you didn't pass any data then of course you don't do either
// Here we pass a bitstream
void clientRPC(char *input, int numberOfBitsOfData, PlayerID sender)
{
	// Since we were passed a bitstream, to make things easier to parse convert the input into into a BitStream.
	// We can either do this in the constructor which takes the data and the number of BYTES
	// BitStream b(input, (numberOfBitsOfData-1)/8+1);
	// or we can do it by writing out the bits
	// BitStream b;
	// b.WriteBits(input, numberOfBitsOfData);

	// Here I will do the constructor version because it is easier.  numberOfBitsOfData is always > 0
	// The third parameter of false means don't internally copy input but just maintain a pointer to it.  This is fine because we never change it
	BitStream b(input, (numberOfBitsOfData-1)/8+1, false);
	char name[200];

	// The length of the person's name.  It is critical to read the same types in the same order using the same level of compression as we wrote them
	unsigned char nameLength;
	b.Read(nameLength);
	if (b.Read(name, nameLength)==false) // Name is not null terminated
	 // Note we always check if the read functions return false.  This is important in a real example in case hackers or programmer error causes too-short bitstreams
	 // You would probably also log this event or throw an assert in a real game
		return;

	name[nameLength]=0; // Name is now null terminated

	printf("In clientRPC:\n");
	printf("Name is %s\n", name);
    
	unsigned char age;
	// We used WriteCompressed for the age, so have to use ReadCompressed to get it
	if (b.ReadCompressed(age)==false)
		return;

	printf("Age is %i\n", (int)age);

    bool wroteEmploymentStruct;
	if (b.Read(wroteEmploymentStruct)==false)
		return;

	if (wroteEmploymentStruct)
	{
		printf("We are employed.\n");

		EmploymentStruct employmentStruct;
		// Reading a struct consists of reading a byte stream and casting it
		if (b.Read((char*)&employmentStruct, sizeof(EmploymentStruct))==false)
			return;

		printf("Salary is %i.  Years employed is %i\n", employmentStruct.salary, (int)employmentStruct.yearsEmployed);
	}
	else
		printf("We are between jobs :)\n");
}
int main(void)
{
	RakClientInterface *rakClient=RakNetworkFactory::GetRakClientInterface();
	RakServerInterface *rakServer=RakNetworkFactory::GetRakServerInterface();
	Multiplayer<RakClientInterface> rakClientMultiplayer;
	Multiplayer<RakServerInterface> rakServerMultiplayer;

	// Defined in NetworkTypes.h.
	// You can register a function anytime
	REGISTER_AS_REMOTE_PROCEDURE_CALL(rakClient, clientRPC);

	rakServer->Start(32,0,30,2000);
	rakClient->Connect("127.0.0.1", 2000, 2100, 0, 30);

	printf("Hit space to send the data or 'q' to quit.\n");
	char ch=0;
	while (1)
	{
		rakClientMultiplayer.ProcessPackets(rakClient);
		rakServerMultiplayer.ProcessPackets(rakServer);
		if (kbhit())
			ch=getch();

		if (ch==' ')
		{
			BitStream outgoingBitstream;
			char text[200];
			unsigned char age;
		
			printf("Enter your name.\n");
			gets(text);
			// Write the number of characters of the name into the bitstream
			// We put the unsigned char cast to use the overload that writes 1 byte.
			outgoingBitstream.Write((unsigned char)strlen(text));

			// Now write the name to the bitstream.  This overload takes a char* and the number of bytes to write
			outgoingBitstream.Write(text, (int) strlen(text));

			printf("Enter your age.\n");
			gets(text);
			age=atoi(text);

			// Write age to the bitstream.  age is an unsigned char so only uses one byte.  However, since the range of
			// age is probably pretty low compared to the range of the variable, we use a compressed write.  This can reduce the number
			// of bits used.
			outgoingBitstream.WriteCompressed(age);

			// Now demonstrate dynamic packets by choosing one of two paths in which to write different amounts / types of data.
			printf("Are you employed (y/n)?\n");
			if (getch()=='y')
			{
				outgoingBitstream.Write(true); // Writing a bool takes 1 bit

				// Read some data into a struct
				EmploymentStruct employmentStruct;
				printf("What is your salary (enter a number only)?\n");
				gets(text);
                employmentStruct.salary = atoi(text);
				printf("How many years have you been employed (enter a number only)?\n");
				gets(text);
				employmentStruct.yearsEmployed = atoi(text);

				// We could have written the salary and years employed directly, but illustrate that we can also write structs to a bitstream
				// We cast the string to a char* and write out the number of bytes that it is
				outgoingBitstream.Write((char*)&employmentStruct, sizeof(EmploymentStruct));
				// We're done writing to the struct
			}
			else
			{
				outgoingBitstream.Write(false); // Writing a bool takes 1 bit
				// We're done writing to the struct.  Compare this to the example above - we wrote quite a bit less.
			}

			// RPC functions as well as send can take bitstreams directly
			rakServer->RPC("clientRPC",&outgoingBitstream, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true, false); // broadcast to everyone, which happens to be our one client

			ch=0;
		}
		else if (ch=='q')
			break;

#ifdef _WIN32
		Sleep(30);
#else
		usleep(30 * 1000);
#endif
	}

	// This is not necessary since on shutdown everything is unregistered.  This is just here to show usage
	UNREGISTER_AS_REMOTE_PROCEDURE_CALL(rakClient, clientRPC);

	RakNetworkFactory::DestroyRakClientInterface(rakClient);
	RakNetworkFactory::DestroyRakServerInterface(rakServer);

	return 0;
}