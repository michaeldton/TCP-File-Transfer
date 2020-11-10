# Author: Michael Ton
# Course: CS372
# Assignment: Project 2
# Description: Client-side implementation for file transfer using TCP connection with server

import socket
import select
import sys
import signal
import os

# Function connects server to socket
def connectTCP():
	# Append address to server name
	server_name = sys.argv[1] + ".engr.oregonstate.edu"
	
	# Get port from command line argument as integer
	server_port = int(sys.argv[2])

	# Connect socket to server and return socket
	client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	client_socket.connect((server_name, server_port))

	return client_socket

# Function receives socket and port receiving data from server, processing data received appropriately based on command
def retrieveData(dataSocket, data_port):
	# Get string containing server host and data port
	server = sys.argv[1] + ":" + data_port

	# If directory list was requested, display message and print directory items
	if sys.argv[3] == "-l":
		print "Receiving directory structure from " + server
		index = dataSocket.recv(100)

		# Display directory items received from server until all have been received
		while index:
			print index
			index = dataSocket.recv(100)
		return
	# If file transfer was requested, display message and receives bytes containing file contents
	else:
		print "Receiving " + sys.argv[4] + " from " + server
		flag = 0

		# Open file using name received in command line and receive content from server
		with open(sys.argv[4], 'w') as newFile:
			content = dataSocket.recv(1024)
		
			# While bytes of data remain, write content to file and receive next package
			while content:
				# If file was not found and message was sent, print and set flag
				if content == "FILE NOT FOUND":
					print server + " says " + content
					flag = 1
				else:
					newFile.write(content)
								
				content = dataSocket.recv(1024)

		# Display transfer completion is successful; clean up created file if could not be found on server side
		if flag == 0:
			print "File transfer complete."
		# Method for removal of file from dummies.com - "How to delete a file in Python"
		elif flag == 1:
			os.remove(sys.argv[4])

# Send arguments to server to begin file transfer
def transfer(client_socket, data_port):
	# Initialize command, address, and port from command line arguments
	command = sys.argv[3]
	addr = sys.argv[1] 
	port_num = int(data_port)

	# Send port data will be received on and receive validation from server
	client_socket.send(data_port)
	client_socket.recv(1024)

	# Send command and address to server
	client_socket.send(command)
	client_socket.send(addr)
	client_socket.recv(1024)

	# If requesting a file transfer, send file name to server
	if command == "-g":
		client_socket.send(sys.argv[4])

	# Create the data socket that will be used to receive data from the server
	server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server_socket.bind(('', port_num))
	server_socket.listen(1)
	dataSocket, address = server_socket.accept()

	retrieveData(dataSocket, data_port)	

	dataSocket.close()

if __name__ == "__main__":
	# Validate command line arguments, return error message with usage if invalid input received
	# If valid, determine port number using appropriate argument from command line
	if len(sys.argv) == 5 and sys.argv[3] == "-l":
		data_port = sys.argv[4]
	elif len(sys.argv) == 6 and sys.argv[3] == "-g":
		data_port = sys.argv[5]
	else:
		print "Usage: python ftclient.py (host server) (host port) (command) (filename) (data port)"
		exit(1)

	# Connect socket and begin data transfer with server
	client_socket = connectTCP()
	transfer(client_socket, data_port)
