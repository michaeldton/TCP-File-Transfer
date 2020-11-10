Author: Michael Ton
Assignment: Project 2

Instructions:

Project 2 is a file transfer application using a TCP connection. To begin, unzip the .zip and place ftserver.c and ftclient.py files in desired directories. User may enter a command using the client. Valid commands are -l to request directory of the server, or -g (filename) to request a file transfer.

To compile ftserver, use the following command: gcc -o ftserver ftserver.c

Usage:

Server: ./ftserver (port) 

Client (directory listing): python ftclient.py (host) (host port) (command) (data port)
Client (file transfer): python ftclient.py (host) (host port) (command) (filename) (data port)

Example usage:

	1. Server started on flip2 with: ./ftserver 30020
	2. Client started on flip3 to request directory list using: python ftclient.py flip2 30020 -l 30021
	3. Client started on flip3 to request file "sample.txt" using: python ftclient.py flip2 30020 -g sample.txt 30021

To terminate the server, send SIGINT using ctrl+C.
