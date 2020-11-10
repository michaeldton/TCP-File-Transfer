/*
 * Name: Michael Ton
 * Course: CS372
 * Assignment: Project 2
 * Description: Server-side implementation for file transfer using TCP connection with client
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#define SA struct sockaddr

const int FILE_MAX = 100;

// Adapted from Beeg's Guide
struct addrinfo* Info(char* address, char* port)
{
	struct addrinfo hints;
	struct addrinfo* server;
	int status;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(address, port, &hints, &server)) != 0)
	{
		fprintf(stderr, "Error.\n", gai_strerror(status));
		exit(1);
	}

	return server;
}

// Function iterates through current working directory and adds names of each file to array passed as parameter before returning number of files
// Adapted from CS344 lectures and own assignments in CS344
int initFiles(char** fileList)
{
	int count = 0;
	DIR* cur;
	struct dirent* fileInDir;
	char cwd[PATH_MAX+1];

	// Get name of current directory and open
	getcwd(cwd, sizeof(cwd));
	cur = opendir(cwd);

	// Iterate until all files have been checked
	while((fileInDir = readdir(cur)) != NULL)
	{
		// If current file is DT_REG (regular file), add file name to current index in array and increment count
		if(fileInDir->d_type == DT_REG)
		{
			strcpy(fileList[count], fileInDir->d_name);
			count++;
		}
	}

	// Close directory
	closedir(cur);
	
	return count;
}

// Function receives addrinfo struct of connecting client, creates and connects socket before returning socket
// Adapted from Beeg's Guide and previous CS372 assignments
int connectSocket(struct addrinfo* client)
{
	int sockfd = socket(client->ai_family, client->ai_socktype, 0);

	// Error handling if socket fails to be created
	if(sockfd == -1)
	{
		printf("Socket creation failed\n");
		exit(1);
	}

	int status = connect(sockfd, client->ai_addr, client->ai_addrlen);

	// Error handling if connection fails
	if(status == -1)
	{
		printf("Socket connection failed\n");
		exit(1);	
	}

	return sockfd;
}

// Function handles data transfer
void transfer(int sockfd, char* command, char* address, char* port)
{
	// Send message to client that command is valid
	send(sockfd, "valid", 5, 0);

	struct addrinfo* client = Info(address, port);
	char** fileList = malloc(FILE_MAX * sizeof(char*));
	int i, numFiles;	

	// Allocate memory for array
	for(i = 0; i < FILE_MAX; i++)
	{
		fileList[i] = malloc(FILE_MAX * sizeof(char));
		memset(fileList[i], 0, sizeof(fileList[i]));	
	}

	// Initialize number of files in directory
	numFiles = initFiles(fileList);

	// If command to send directory list is received
	if(strcmp(command, "-l") == 0)
	{
		printf("List directory requested on port %s\n", port);
		printf("Sending directory contents to %s:%s\n", address, port);

		// Create a data socket with client
		int dataSocket = connectSocket(client);

		// Iterate through list of files, sending each one to data socket	
		for(i = 0; i < numFiles; i++)
		{
			send(dataSocket, fileList[i], FILE_MAX, 0);
		}

		close(dataSocket);
	}
	// If command to send specified text file is received
	else if(strcmp(command, "-g") == 0)
	{
		int isValid = 0;
		char fileName[FILE_MAX];
	
		// Initialize array containing file name and receive file name from client
		memset(fileName, 0, sizeof(fileName));
		recv(sockfd, fileName, sizeof(fileName) - 1, 0);

		printf("File \"%s\" requested on port %s\n", fileName, port);

		// Iterate through array to find requested file
		for(i = 0; i < numFiles; i++)
		{
			// If the file is found, change flag to 1 and initialize i outside of loop range to break early
			if(strcmp(fileList[i], fileName) == 0)
			{
				isValid = 1;
				i = numFiles + 1;
			}
		}

		// If the file has been found (isValid = 1), begin sending data
		if(isValid)
		{
			printf("Sending \"%s\" to %s:%s\n", fileName, address, port);

			// Create a data socket with client
			int dataSocket = connectSocket(client);

			// Open the specified file and use fseek() and ftell() to determine the size of the file's contents in bytes
			FILE *fp = fopen(fileName, "r");
		
			fseek(fp, 0L, SEEK_END);
			size_t size = ftell(fp);
			fseek(fp, 0L, SEEK_SET);

			fclose(fp);

			// Initialize the buffer that will contain the current batch of file contents and variable tracking remaining bytes to send
			char buffer[size];
			size_t bytesLeft = size;
	
			int fileDescriptor = open(fileName, O_RDONLY);

			// Iterate until all bytes have been sent to socket
			// Written using reference to linux/C documentation on man7.org and StackOverflow
			while(bytesLeft > 0)
			{
				memset(buffer, 0, sizeof(buffer));

				// Send content to buffer and decrement remaining bytes by number of bytes read
				int bytesRead = read(fileDescriptor, buffer, sizeof(buffer));
				bytesLeft -= bytesRead;

				// If read failed or incorrect bytes read, display error
				if(bytesRead < 0)
				{
					printf("Error reading file\n");
					return;
				}
				
				// package will contain the data to be sent to client
				void *package = buffer;

				// Iterate while current content stored in buffer remains
				while(bytesRead > 0)
				{
					// Get number of bytes sent to socket
					int bytesWritten = send(dataSocket, package, sizeof(buffer), 0);

					// If transmission failed or incorrect bytes sent, display error
					if(bytesWritten < 0){
						fprintf(stderr, "Error writing to socket\n");
						return;
					}

					// Otherwise, calculate bytes remaining for current package and get position for next bytes to write
					bytesRead -= bytesWritten;
					package += bytesWritten;
				}
			}

			close(dataSocket);
		}
		// If file could not be located, print server-side message and send error to client
		else 
		{
			int dataSocket = connectSocket(client);

			printf("File not found. Sending error message to %s:%s\n", address, port);

			char* error = "FILE NOT FOUND";
			send(dataSocket, error, 14, 0);

			close(dataSocket);
		}
	}

	// Free any allocated memory remaining
	for(i = 0; i < numFiles; i++)
	{
		if(fileList[i] != NULL)
			free(fileList[i]);
	}

	free(fileList);
	freeaddrinfo(client);
}

// Function receives port, command, and address from client, determining command validity and beginning transfer if valid
// Adapted using various resources, including: CS372 lectures, assignments, Beeg's guide, and StackOverflow
void GetConnection(int sockfd, struct sockaddr_in server)
{
	int len;
	struct sockaddr_storage client;	
	socklen_t size;
	char port[FILE_MAX];
	char address[FILE_MAX];
	char command[FILE_MAX];

	// Server can only be ended when SIGINT has been received
	while(1)
	{
		// Get size of client struct and accept connectoin with client
		size = sizeof(client);
		int cur_sockfd = accept(sockfd, (struct sockaddr*)&client, &size);
		
		if(cur_sockfd == -1)
		{
			fprintf(stderr, "Unable to accept connection.\n");
			exit(1);
		}

		// Initialize port, command, and address
		memset(port, 0, sizeof(port));	
		memset(command, 0, sizeof(command));
		memset(address, 0, sizeof(address));	

		// Receive port from client and send back valid message
		recv(cur_sockfd, port, sizeof(port) - 1, 0);
		send(cur_sockfd, "valid", 6, 0);

		// Receive command and address from client
		recv(cur_sockfd, command, sizeof(command) - 1, 0);	
		recv(cur_sockfd, address, sizeof(address) - 1, 0);

		printf("Connection from: %s\n", address);

		// If commands are valid, call transfer function
		if(strcmp(command, "-l") == 0 || strcmp(command, "-g") == 0)
		{
			transfer(cur_sockfd, command, address, port);
		}
	
		close(cur_sockfd);		

		fflush(stdin);
		fflush(stdout);	
	}
}

// Function to listen for socket
// Adapted from Beeg's Guide and CS372 course material
void ListenSocket(int sockfd)
{
	if(listen(sockfd, 5) != 0)
	{
		close(sockfd);
		fprintf(stderr, "Could not listen to socket.\n");
		exit(1);
	}
//	else
//	{
//		printf("Successful listen.\n");
//	}
}

int main(int argc, char* argv[])
{
	// If command line arguments are invalid, display proper usage
	if(argc != 2)
	{
		fprintf(stderr, "Usage: ./ftserver (port number)\n");
		exit(1);
	}

	// Create socket and sockaddr_in used for client and server
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server;

	// Receive port number from command line argument
	int port;
	sscanf(argv[1], "%d", &port);

	// Error handling if unable to create socket
	if(sockfd == -1)
	{
		printf("Unable to create socket.\n");
		exit(0);
	}
	else
	{	
		printf("Server open on %d\n", port);	
	}

	// Assign address and port to structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	// Bind socket, return error if unable to
	if(bind(sockfd, (SA*)&server, sizeof(server)) != 0)
	{
		printf("Unable to bind socket.\n");
		exit(0);
	}	
	else
	{
		printf("Socket bind successful.\n");
	}
	
	// Call functions to listen to socket and form connection
	ListenSocket(sockfd);
	GetConnection(sockfd, server);

	close(sockfd);

	return 0;
}
