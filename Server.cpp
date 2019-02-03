// CSCI 3010: Computer Networks
// Assignment 1
// Group: Daniel Meeks, Merlyn Guevara, Christopher Castro, Bryant Rochin

/*
	This program is the server of a client program. It communicates with the client to listen to 
	and accept request to either send the server data file version or to send the server data 
	to update the clients data.
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

// Closes the socket and performs the WSACleanup.
void cleanup(SOCKET socket);

// Returns the version number from the data file.
int getLocalVersion();

// Uploads data from the server to the client
// Parameter is a socket for communication.
// Returns an error code.
  // Case Error code = 0, No errors. 
  // Case Error code = 1, Error.
int uploadUpdate(SOCKET& socket);

// Reads the two data values from the data file.
// When the function ends, num1 and num2 will be holding the
// two values that were read from the file.
void readData(int& num1, int& num2);

int main()
{
	WSADATA		wsaData;
	SOCKET		listenSocket;
	SOCKET	acceptSocket;
	SOCKADDR_IN	serverAddr;
	int			localVersion = 0;
	int			request = 0;
	int			requestHandled = 0;

	// Loads Windows DLL (Winsock version 2.2) used in network programming
	if ( WSAStartup( MAKEWORD(2, 2), &wsaData ) != NO_ERROR )
	{
		cerr << "ERROR: Problem with WSAStartup\n";
		return 1;
	}

	// Create a new socket to listen for client connections
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if ( listenSocket == INVALID_SOCKET )
	{
		cerr << "ERROR: Cannot create socket\n";
		WSACleanup();
		return 1;
	}

	// Setup a SOCKADDR_IN structure which will be used to hold address
	// and port information
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons( PORT );	
	inet_pton(AF_INET, IPADDR, &serverAddr.sin_addr);

	// Attempt to bind to the port
	if ( bind ( listenSocket, (SOCKADDR*) &serverAddr, sizeof(serverAddr) ) == SOCKET_ERROR )
	{
		cerr << "ERROR: Cannot bind to port\n";
		cleanup( listenSocket );		
		return 1;
	}

	// Start listening for incoming connections
	if ( listen ( listenSocket, 1 ) == SOCKET_ERROR )
    {
        cerr << "ERROR: Problem with listening on socket\n";
        cleanup( listenSocket );
        return 1;
    }

	// Get server version of data file
	localVersion = getLocalVersion();

	cout << "Update server\n";
	cout << "Current data file version: v" << localVersion << "\n";
	cout << "Running on port number " << PORT << "\n";

	while( true )
	{
		// Server hotswap requirement
		if( requestHandled % 5 == 0 ){
			localVersion = getLocalVersion();
		}

		cout << "\nWaiting for connections...\n";
		
		// Accept incoming connection
		acceptSocket = accept( listenSocket, NULL, NULL );

		cout << "Connection received\n";

		// Receive client request
		int recvReq = recv(acceptSocket, (char*)&request, sizeof(int), 0);

		if( recvReq <= 0 ){
			cerr << "ERROR: Failed to receive client request\n";
			
			// Close listening socket since it is no longer needed
			closesocket(listenSocket);
			cleanup(acceptSocket);
			return 1;
		}

		// Request for server version from client
		if (request == QUERY) {
			cout << "	Request for current version number: v" << localVersion << "\n";

			// Send version number
			int sendV = send(acceptSocket, (char*)&localVersion, sizeof(int), 0);

			if (sendV == SOCKET_ERROR)
			{
				cerr << "ERROR: Failed to send version number\n";
				closesocket(listenSocket);
				cleanup(acceptSocket);
				return 1;
			}

			// Close connection
			closesocket(acceptSocket);
			cout << "	Connection closed\n";
		}

		// Request to update from client
		else if (request == UPDATE) {
			cout << "	Request for update: v" << localVersion << "\n";

			// Send data to client
			int errorCode = uploadUpdate(acceptSocket);

			if (errorCode == 1) { return 1; }
			
			// Close connection
			closesocket(acceptSocket);
			cout << "	Connection closed\n";
		}

		// Invalid request
		else {
			cerr << "ERROR: request from connection is invalid\n";

			// Close all Sockets
			closesocket(listenSocket);
			cleanup(acceptSocket);
			return 1;
		}

		requestHandled++;

		cout << "Total request handled : " << requestHandled << "\n";
}

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

void cleanup(SOCKET socket)
{
	closesocket(socket);
	WSACleanup();
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

int uploadUpdate(SOCKET& socket){
	int firstNum = 0;
	int secondNum = 0;

	// Read data from file
	readData(firstNum, secondNum);

	// Send first number
	int sendNum1 = send(socket, (char*)&firstNum, sizeof(int), 0);

	if (sendNum1 == SOCKET_ERROR)
	{
		cerr << "ERROR: Failed to send data\n";
		cleanup(socket);
		return 1;
	}

	// Send second number
	int sendNum2 = send(socket, (char*)&secondNum, sizeof(int), 0);

	if (sendNum2 == SOCKET_ERROR)
	{
		cerr << "ERROR: Failed to send data\n";
		cleanup(socket);
		return 1;
	}

	return 0;
}