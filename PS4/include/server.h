// This file is a part of OpenRTE
// Version: 1.0
// Author: TheoryWrong
// Thanks to: CTurt, Zecoxao, wskeu, BadChoicez, wildcard, Z80 and all people contribuing to ps4 scene
// Also thanks to: Dev_Shootz, MsKx, Marbella, JimmyModding, AZN, MarentDev (Modder/Tester)
// This file is under GNU licence

#ifndef SERVER_H
#define SERVER_H

#define SERVER_PORT 13003 // Server port
#define SOCKET_TIMEOUT 20
#define TRANSFERT_RATE 20000

typedef struct {
	int magic;
	int size;
} Header;

void* recv_all(int client, int* size);
int send_all(int client, void* data, int size);
void socket_error(char* name);
void *client_t(void* c);
void *server_t(void *args);

#endif