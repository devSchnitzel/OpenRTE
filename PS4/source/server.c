// This file is a part of OpenRTE
// Version: 1.0
// Author: TheoryWrong
// Thanks to: CTurt, Zecoxao, wskeu, BadChoicez, wildcard, Z80 and all people contribuing to ps4 scene
// Also thanks to: Dev_Shootz, MsKx, Marbella, JimmyModding, AZN, MarentDev (Modder/Tester)
// This file is under GNU licence

#include <errno.h>
#define _KERNEL
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <string.h>
#include <ps4/socket.h>
#include <ps4/stream.h>
#include <ps4/kernel.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <debugnet.h>
#include <pthread.h>
#include <sce/kernel.h>
#include <sce/types/kernel.h>
#include "openrte.h"
#include "server.h"

// recevie all data
void* recv_all(int client, int* size) {
	void* data = malloc(0);
	int data_size = 0;

	// Ont reçois le header du packet
	Header h;
	int header_s = recv(client, &h, sizeof(Header), 0);

	if (header_s > 0) {
		if (h.magic == 1337) {
			debugNetPrintf(INFO, "%i bytes to recevied\n", h.size);

			// Ont reçois les donnée
			debugNetPrintf(INFO, "Getting data ...\n");

			int buffer_s = 0;
			while (data_size < h.size) {
				int b_size = TRANSFERT_RATE;
				if ((h.size - data_size) < TRANSFERT_RATE) {
					b_size = h.size - data_size;
				}

				char buffer[b_size];
				buffer_s = recv(client, &buffer, b_size, 0);

				if (buffer_s > 0) {
					data = realloc(data, data_size + b_size);
					memcpy(data + data_size, &buffer, b_size);
					data_size += buffer_s;
				}
				else if (buffer_s == 0) {
					debugNetPrintf(DEBUG, "Socket: Gracefully disconnected\n");
					return (void*)0xdeadbeef;
				}
				else {
					debugNetPrintf(ERROR, "Socket: error detected\n");
					socket_error("recv_all()");
					return (void*)0xdeadbeef;
				}
			}
		} else {
			debugNetPrintf(INFO, "The magic header is not correct\n");
			*size = 0;
			return (void*)0x0;
		}
	} else if (header_s == 0) {
		debugNetPrintf(DEBUG, "Socket: Gracefully disconnected\n");
		return (void*)0xdeadbeef;
	} else {
		debugNetPrintf(ERROR, "Socket: error detected\n");
		socket_error("recv_all()");

		if (errno != EAGAIN) {
			return (void*)0xdeadbeef;
		}
	}

	debugNetPrintf(DEBUG, "End of transmission\n");
	*size = data_size;
	return data;
}

// send all data
int send_all(int client, void* data, int size) {
	Header h;
	h.magic = 1337;
	h.size = size;

	// Ont envoi la taille du packet
	debugNetPrintf(DEBUG, "Sending header ...\n");
	debugNetPrintf(DEBUG, "Size of header: %i bytes\n", sizeof(Header));
	debugNetPrintf(DEBUG, "Size of data: %i bytes\n", h.size);

	send(client, &h, sizeof(Header), 0);

	// Ont envoi les donnée
	debugNetPrintf(DEBUG, "Sending data.\n");
	send(client, data, size, 0);

	debugNetPrintf(DEBUG, "End of transmission\n");
	return 0;
}

// Get the good error message
void socket_error(char* name) {
	switch (errno) {
		case ECONNRESET:
		{
			debugNetPrintf(ERROR, "%s: ECONNRESET (Connexion reset by peer)\n", name);
			break;
		}
		case EAGAIN:
		{
			debugNetPrintf(ERROR, "%s: EAGAIN (Timeout)\n", name);
			break;
		}
		case ENOTCONN:
		{
			debugNetPrintf(ERROR, "%s: ENOTCONN (Not connected)\n", name);
			break;
		}
		case EBADF:
		{
			debugNetPrintf(ERROR, "%s: EBADF (Invalid descriptor)\n", name);
			break;
		}
		case EINTR:
		{
			debugNetPrintf(ERROR, "%s: EINTR (Accept was interupted)\n", name);
			break;
		}
		case ENFILE:
		{
			debugNetPrintf(ERROR, "%s: ENFILE (Descriptor table is full)\n", name);
			break;
		}
		case ENOTSOCK:
		{
			debugNetPrintf(ERROR, "%s: ENOTSOCK (The descriptor is a file, not a socket)\n", name);
			break;
		}
		case EINVAL:
		{
			debugNetPrintf(ERROR, "%s: EINVAL (Listen is not called)\n", name);
			break;
		}
		default:
		{
			debugNetPrintf(ERROR, "%s: Unknown error ! (0x%08x)\n", name, errno);
			break;
		}
	}
}

// Client thread
void *client_t(void* c) {
	int client = *((int *) c);

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	setsockopt(c, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));

	debugNetPrintf(INFO, "New client: %04x\n",  client);

	int size = 0;

	while (1) {
		debugNetPrintf(INFO, "Waiting client ...\n");
		void* data = recv_all(client, &size);

		if (data == (void*)0xdeadbeef) {
			debugNetPrintf(ERROR, "Client: error detected, closing client socket ...\n");
			break;
		}

		if (size == 0) {
			debugNetPrintf(INFO, "Client: no data detected, passed\n");
		}

		if (size > 0) {
			debugNetPrintf(INFO, "Client: new command received (%i bytes)\n", size);
			execute_action(client, data, size);
		}
	}

	debugNetPrintf(INFO, "Client %04x disconnected.\n", client);
	close(client);

	return NULL;
}

// Server for thread
void *server_t(void *args) {
	int server_sid;
	int client;

	// Create server
	int serv_r = ps4SocketTCPServerCreate(&server_sid, SERVER_PORT, 10);

	if (serv_r != PS4_OK) {
		debugNetPrintf(ERROR, "Unable to create server !\n");
		switch (serv_r) {
			case -1: {
				debugNetPrintf(ERROR, "Unable to create socket.\n");
				socket_error("server_thread(TCPServerCreate)");
				break;
			}
			case -2: {
				debugNetPrintf(ERROR, "Unable to bind socket.\n");
				socket_error("server_thread(TCPServerCreate)");
				break;
			}
			case -3: {
				debugNetPrintf(ERROR, "Unable to listen socket.\n");
				socket_error("server_thread(TCPServerCreate)");
				break;
			}
			default: {
				debugNetPrintf(ERROR, "Unknown error !\n");
			}
		}
		return NULL;
	}

	// Wait client
	while (1) {
		debugNetPrintf(DEBUG, "Waiting new client ...\n");
		client = accept(server_sid, NULL, NULL);

		debugNetPrintf(DEBUG, "new client: %i\n", client);

		struct timeval tv;
		tv.tv_sec = SOCKET_TIMEOUT; 
		tv.tv_usec = 0;
		setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

		if (client < 0) {
			debugNetPrintf(ERROR, "Unable to accept socket.\n");
			socket_error("server_thread(accept)");
		} else {
			// Create client thread
			debugNetPrintf(DEBUG, "New client thread created\n");
			pthread_t c_thread;
			pthread_create(&c_thread, NULL, client_t, (void *)&client);
		}
	}
	
	close(server_sid);
	debugNetPrintf(INFO, "OpenRTE Server closed !\n");

	return NULL;
}