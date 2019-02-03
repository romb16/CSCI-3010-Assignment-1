// CSCI 3010: Computer Networks
// Assignment 1
// Group: Daniel Meeks, Merlyn Guevara, Christopher Castro, Bryant Rochin

/*
	This program is the client of a server program and serves as a calculator that
	reads data from a file and calculates the sum. The file contains the version of the 
	calculator and two numbers. The client communicates with the server to check the 
	server version of the data and updates the data file of the client if needed. 
*/

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include "FileHelper.h"
using namespace std;

#pragma comment(lib, "Ws2_32.lib")

const char FILENAME[] = "data.bin";
const char IPADDR[] = "127.0.0.1";
const int  PORT = 50000;
const int  QUERY = 1;
const int  UPDATE = 2;

// Returns the version number from the data file.
int getLocalVersion();

// Reads the two data values from the data file.
// When the function ends, num1 and num2 will be holding the
// two values that were read from the file.
void readData(int& num1, int& num2);

// Closes the socket and performs the WSACleanup.
void cleanup(SOCKET socket);

// Downloads data from server to client.
// Parameters are the version number to write to file and
// a socket for communication.
// Returns an error code.
  // Case Error code = 0, No errors. 
  // Case Error code = 1, Error.
int downloadUpdate(int& version, SOCKET& socket);

int main()
{
	WSADATA		wsaData;
	SOCKET		mySocket;				
	SOCKADDR_IN	serverAddr;
	int			sum;
	int			num1 = 0;
	int			num2 = 0;	
	int			localVersion = 0;
	int			serverVersion = 0;

	// Client code starts here: Request to check server data version and update if necessary

	// Loads Windows DLL (Winsock version 2.2) used in network programming
	if ( WSAStartup( MAKEWORD(2, 2), &wsaData ) != NO_ERROR )
	{
		cerr << "ERROR: Problem with WSAStartup\n";
		return 1;
	}

	// Create a new socket for communication
	mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (mySocket == INVALID_SOCKET)
	{
		cerr << "ERROR: Cannot create socket\n";
		WSACleanup();
		return 1;
	}

	// Get client version of data file
	localVersion = getLocalVersion();

	// Setup a SOCKADDR_IN structure which will be used to hold address
	// and port information for the server.
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);	
	inet_pton(AF_INET, IPADDR, &serverAddr.sin_addr);

	// Try to connect
	if ( connect( mySocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr) ) == SOCKET_ERROR )
	{
		cerr << "ERROR: Failed to connect\n";
		cleanup(mySocket);
		return 1;
	}

	cout << "Checking for updates... \n";

	// Send request for server version
	int sendReqV = send(mySocket, (char*)&QUERY, sizeof(QUERY), 0);

	if (sendReqV == SOCKET_ERROR) 
	{
		cerr << "ERROR: Failed to query server version\n";
		cleanup(mySocket);
		return 1;
	}

	// Receive server version
	int recvVersion = recv(mySocket, (char*)&serverVersion, sizeof(serverVersion), 0);

	if(recvVersion <= 0){
		cerr << "ERROR: Failed to receive server version\n";
		cleanup(mySocket);
		return 1;
	}

	// Communication ends if client version is same as server version
	if(localVersion == serverVersion){
		cout << "No updates found\n";
		cleanup(mySocket);
	}

	// Update client data if the server version is not the same
	else {
		cout << "Downloading updates...\n";

		// Create socket
		mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (mySocket == INVALID_SOCKET)
		{
			cerr << "ERROR: Cannot create socket\n";
			WSACleanup();
			return 1;
		}

		// Connect
		if (connect(mySocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			cerr << "ERROR: Failed to connect\n";
			cleanup(mySocket);
			return 1;
		}
		
		// Request update
		int sendReqU = send(mySocket, (char*)&UPDATE, sizeof(int) + 1, 0);

		if (sendReqU == SOCKET_ERROR)
		{
			cerr << "ERROR: Failed to request update\n";
			cleanup(mySocket);
			return 1;
		}

		// Download update from server and update client data
		int errorCode = downloadUpdate(serverVersion, mySocket);

		if (errorCode == 1) { return 1; }

		cout << "Update finished\n";
		cleanup(mySocket);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Main purpose of the program starts here: read two numbers from the data file and calculate the sum
	localVersion = getLocalVersion();
	cout << "\nSum Calculator Version " << localVersion << "\n\n";

	readData(num1, num2);	
	sum = num1 + num2;
	cout << "The sum of " << num1 << " and " << num2 << " is " << sum << endl;

	return 0;
}

int getLocalVersion()
{
	ifstream dataFile;
	openInputFile(dataFile, FILENAME);

	int version = readInt(dataFile);
	dataFile.close();

	return version;
}

void readData(int& num1, int& num2)
{
	ifstream dataFile;
	openInputFile(dataFile, FILENAME);

	// Read the version number and discard it
	int tmp = num1 = readInt(dataFile);

	// Read the two data values
	num1 = readInt(dataFile);
	num2 = readInt(dataFile);

	dataFile.close();
}

void cleanup(SOCKET socket)
{
	closesocket(socket);
	WSACleanup();
}

int downloadUpdate(int& version, SOCKET& socket)
{
	int firstNum = 0;
	int	secondNum = 0;

	// Receive the first number
	int recvNum1 = recv(socket, (char*)&firstNum, sizeof(int), 0);

	if (recvNum1 <= 0)
	{
		cerr << "ERROR: Failed to receive data\n";
		cleanup(socket);
		return 1;
	}
	
	// Receive the second number
	int recvNum2 = recv(socket, (char*)&secondNum, sizeof(int), 0);
	
	if (recvNum2 <= 0)
	{
		cerr << "ERROR: Failed to receive data\n";
		cleanup(socket);
		return 1;
	}

	// Write the new data to file
	ofstream dataFile;
	openOutputFile(dataFile, FILENAME);
	writeInt(dataFile, version);
	writeInt(dataFile, firstNum);
	writeInt(dataFile, secondNum);
	dataFile.close();
	return 0;
}