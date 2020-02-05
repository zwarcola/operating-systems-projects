/*
 * Alex Benasutti and Alex Van Orden
 * CSC345-01
 * Network Programming
 * main_server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT_NUM 1004
#define ROOM_MAX 255

int room_tot = 0;		 			// Integer to hold the total amount of available rooms
int room_list[ROOM_MAX+1] = {0}; 	// Array to hold the amount of people in each room

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/* Declare user structure */
typedef struct _USR {
	int clisockfd;		// socket file descriptor
	char username[12]; 	// user name
	int room;
	int color;
	struct _USR* next;	// for linked list queue
} USR;

/* Allow linked lists of users for user traversal */
USR *head = NULL; // traversal
USR *tail = NULL; // adding new client

void print_client_list();
int choose_color();

/* add_tail - Add a new client/user to the end of the linked list */
void add_tail(int newclisockfd, char* username, int roomNum)
{
	// printf("Adding new user to tail\n");
	if (head == NULL) {
		// printf("Head is NULL\n");
		head = (USR*) malloc(sizeof(USR));
		head->clisockfd = newclisockfd;
		// printf("Adding attributes\n");

		strcpy(head->username, username);
		head->room = roomNum;
		head->color = choose_color();

		// printf("making tail head\n");
		head->next = NULL;
		tail = head;

		// printf("New user - %s\n", tail->username);
	} else {
		// printf("Head is not NULL\n");
		tail->next = (USR*) malloc(sizeof(USR));
		tail = tail->next;

		tail->clisockfd = newclisockfd;
		strcpy(tail->username, username);
		tail->room = roomNum;
		tail->color = choose_color();

		tail->next = NULL;

		// printf("New user - %s\n", tail->username);
	}
}

int choose_color()
{
	int clr_idx_1 = rand() % 7;
	// int clr_idx_2 = rand() % 16;

	// char* color;
	// sprintf(color, "\x1B[%dm", clr_idx_1*16+clr_idx_2);

	// int color = clr_idx_1*16+clr_idx_2;
	int color = clr_idx_1 + 31;
	// printf("COLOR: %d\n", color);

	return color;
}


/* broadcast - send message from client to all clients */
void broadcast(int fromfd, char* message)
{
	// printf("Attempting to broadcast\n");
	// figure out sender address
	struct sockaddr_in cliaddr;
	socklen_t clen = sizeof(cliaddr);
	if (getpeername(fromfd, (struct sockaddr*)&cliaddr, &clen) < 0) error("ERROR Unknown sender!");

	char sendername[12];
	int senderroom;
	int sendercolor;
	USR* cur = head;
	/* Loop to grab username of sender by matching file descriptors */
	while (cur != NULL)
	{
		if(cur->clisockfd == fromfd) // looking for sender
		{
			strcpy(sendername, cur->username);
			senderroom = cur->room;
			sendercolor = cur->color;
			// if(strlcpy(username, cur->username, 12) >= 12)
			// {
			// 	printf("Warning: %s concatenated to %s\n", cur->username, username);
			// }
			break;
		}
		
		cur = cur->next;
	}
	// printf("Got sender: %s\n", sendername);

	// traverse through all connected clients
	cur = head;
	while (cur != NULL) {
		// check if cur is not the one who sent the message
		if (cur->clisockfd != fromfd && cur->room == senderroom) {
			// printf("Printing the message now\n");
			char buffer[512];
			memset(buffer, 0, 512);

			// prepare message
			sprintf(buffer, "\x1B[%dm[%d][%s (%s)]:\x1B[0m %s", sendercolor, senderroom, inet_ntoa(cliaddr.sin_addr), sendername, message);
			// sprintf(buffer, "[%s]:%s", username, message);
			int nmsg = strlen(buffer);

			// send!
			// printf("Sending the message now\n");
			int nsen = send(cur->clisockfd, buffer, nmsg, 0);
			if (nsen != nmsg) error("ERROR send() failed");
		}

		cur = cur->next;
	}
	// printf("Message finished printing\n");
}

void print_client_list()
{
	USR* cur = head;
	
	if(head != NULL)
	{
		printf("Current Connected Clients\n");
		cur = head;
		while (cur != NULL)
		{
			printf("%s\n", cur->username);
			cur = cur->next;	
		}
	}
	else
	{
		printf("No Connected Clients\n");
	}	
}

/* ThreadArgs - Holds client socket file descriptor */
typedef struct _ThreadArgs {
	int clisockfd;
} ThreadArgs;

/* thread_main - main worker function for each thread */
void* thread_main(void* args)
{
	// make sure thread resources are deallocated upon return
	pthread_detach(pthread_self());

	// get socket descriptor from argument
	int clisockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	//-------------------------------
	// Now, we receive/send messages
	char buffer[256];
	memset(buffer, 0, 256);
	int nsen, nrcv;
	USR* cur = head;
	USR* prv = NULL;

	memset(buffer, 0, 256);
	// printf("Waiting to receive first buffer\n");
	nrcv = recv(clisockfd, buffer, 255, 0);
	// printf("Received first buffer\n");
	
	// Very first buffer receieve
	if (nrcv < 0)
	{
		error("ERROR recv() failed");
	}
	else if (nrcv == 0) // when a client disconnects
	{
		// printf("Client disconnected, removing client fd\n");
		// remove file descriptor to disconnected client
		while (cur != NULL)
		{
			if(cur->clisockfd == clisockfd)
			{
				printf("Disconnected Client: (%s)\n", cur->username);

				room_list[(cur->room)- 1]--;

				if(prv == NULL)
				{
					// printf("previous is null\n");
					head = cur->next;	// change head
					free(cur);			// free old head
				}
				else // some node other than the head needs to be removed
				{
					// printf("%s links to %s\n",prv->next->username, cur->next->username);
					prv->next = cur->next;
					free(cur);
				}
				
				break;
			}
			
			prv = cur;
			cur = cur->next;
		}
		print_client_list();
	}


	while (nrcv > 0) {
		// we send the message to everyone except the sender
		broadcast(clisockfd, buffer);

		memset(buffer, 0, 256);
		// printf("Check for new messages\n");
		nrcv = recv(clisockfd, buffer, 255, 0);
		// printf("New buffer received: %s\n", buffer);

		if (nrcv < 0) error("ERROR recv() failed");
		else if (nrcv == 0) // when a client disconnects
		{
			//remove file discriptor to disconected client
			while (cur != NULL)
			{
				if(cur->clisockfd == clisockfd)
				{
					printf("Disconnected Client: (%s)\n", cur->username);

					// printf("Previous List\n");
					// print_client_list();

					// printf("cur->room: %d\n", cur->room);
					// printf("room_list[(cur->room)- 1]: %d\n", room_list[(cur->room)- 1]);
					room_list[(cur->room)- 1]--;

					if(prv == NULL)
					{
						// printf("hmm previous is null\n");
						head = cur->next;
						free(cur);
					}
					else
					{
						// printf("%s links to %s\n",prv->next->username, cur->next->username);
						prv->next = cur->next;
						free(cur);
					}
						
					break;
				}
				
				prv = cur;
				cur = cur->next;
			}
		}
		print_client_list();
	}

	close(clisockfd);
	//-------------------------------

	return NULL;
}


int getRoomNum(int clisockfd, struct sockaddr_in cli_addr, char* room)
{
	int digitFlag = 1;
	int noFlag = 0;
	int roomNum;
	/* Confirm whether every part of the message, besides \n, was an integer */
	for (int i=0;i<strlen(room)-1;i++)
	{
		if (room[i] < 48 || room[i] > 57) // chars 0 to 9 are 48 to 57
		{
			digitFlag = 0;
			break;
		}
	}

	// check if room was submitted as nothing
	if (room[0] == 10)
	{
		digitFlag = 0;
		noFlag = 1;
	}

	// room input is an integer room, check that it exists and return if true
	if (digitFlag)
	{
		// printf("Digit room specified\n");
		roomNum = atoi(room);

		// printf("roomNum: %d\n", roomNum);
		// Check to see if room num is valid
		if (roomNum > 0 && roomNum <= ROOM_MAX && roomNum <= room_tot) 
		{
			memset(room,0,256);
			sprintf(room,"Success\n");
			// Send junk array to client
			int nmsg = strlen(room);
			int nsen = send(clisockfd, room, nmsg, 0);
			if (nsen != nmsg) error("ERROR send() failed");
			return roomNum;
		}
		else // punish the client if invalid
		{
			memset(room,0,256);
			sprintf(room,"Well this is unfortunate\n");
			// Send junk array to client
			int nmsg = strlen(room);
			int nsen = send(clisockfd, room, nmsg, 0);
			if (nsen != nmsg) error("ERROR send() failed");
			return 0;
		}
		// else error("ERROR room number out of range or nonexistent");
	}
	// room input is a string, check that it = "new" and return new room number if true, broadcast to client
	else
	{
		int newFlag = strcmp(room,"new");
		// client asks for new room, create it
		if ((newFlag == 0 && room_tot < ROOM_MAX) || (noFlag == 1 && room_tot == 0))
		{
			// printf("Creating new room\n");
			room_tot += 1;
			roomNum = room_tot;

			// Send message of new chat room back to client
			char buffer[256];
			memset(buffer, 0, 256);
			sprintf(buffer, "Connected to %s with new room number: %d\n", inet_ntoa(cli_addr.sin_addr), roomNum);

			int nmsg = strlen(buffer);
			int nsen = send(clisockfd, buffer, nmsg, 0);
			if (nsen != nmsg) error("ERROR send() failed");

			// Clear this again because we were having text problems
			memset(buffer, 0, 256);

			return roomNum;
		}
		else if (noFlag == 1 && room_tot != 0 && room_tot < ROOM_MAX) // client input nothing, prompt client to choose a room from a list
		{
			// printf("No room specified from client\n");

			// Time to prompt user to choose a room
			int maxLen = 4096;
			// Create buffer array of all possible choices to send to client
			char buffer[4096];
			memset(buffer, 0, 4096);

			// Start putting stuff in the buffer array
			sprintf(buffer, "Server says the following options are available:\n");
			for (int i=0;i<room_tot;i++)
			{
				char room[32];
				int people = room_list[i];

				// Create a string for each room
				if (people > 1 || people == 0) sprintf(room, "\tRoom %d: %d people\n", i+1, people);
				else sprintf(room, "\tRoom %d: %d person\n", i+1, people);

				// Add room string to buffer array
				if (strlen(buffer) + strlen(room) < maxLen) strcat(buffer, room);
				else error("ERROR max length of room decision string exceeded");
			}

			strcat(buffer, "Choose a room number or type [new] to create a new room: ");

			// Send prompt to client and let them choose a room
			int nmsg = strlen(buffer);
			int nsen = send(clisockfd, buffer, nmsg, 0);
			if (nsen != nmsg) error("ERROR send() failed");

			// Get new input from client
			char newbuf[256];
			memset(newbuf, 0, 256);
			int nrcv = recv(clisockfd, newbuf, 255, 0);
			if (nrcv < 0) error("ERROR recv() failed");

			// Get room num using a new call to the function
			return getRoomNum(clisockfd, cli_addr, newbuf);
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	//serv_addr.sin_addr.s_addr = inet_addr("192.168.1.171");	
	serv_addr.sin_port = htons(PORT_NUM);

	int status = bind(sockfd, 
			(struct sockaddr*) &serv_addr, slen);
	if (status < 0) error("ERROR on binding");

	listen(sockfd, 5); // maximum number of connections = 5

	/* Main loop for accepting clients and sending messages */
	while(1) 
	{
		// Accept a new client into the server
		struct sockaddr_in cli_addr;
		socklen_t clen = sizeof(cli_addr);
		int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clen);
		if (newsockfd < 0) error("ERROR on accept");

		// Temporarily link with just-created client to retrieve room number
		char room[256];
		// Clear array for new room number and receive room number from client
		memset(room, 0, 256);
		int nrcv = recv(newsockfd, room, 255, 0);
		if (nrcv < 0) error("ERROR recv() failed");

		// Get either a new or existing ROOM NUMBER
		int roomNum = getRoomNum(newsockfd, cli_addr, room);
		// if (roomNum == 0) error("ERROR getRoomNum() failed");
		// getRoomNum failed - punish the client, not the server
		if (roomNum == 0)
		{
			printf("getRoomNum() failed oh no\n");
			continue;
		}

		// increment number of people in room
		room_list[roomNum-1] += 1;

		// Temporarily link with just-created client to retrieve the user name
		char username[256];
		// Clear array for new username
		memset(username, 0, 256);
		nrcv = recv(newsockfd, username, 255, 0);
		if (nrcv < 0) error("ERROR recv() failed");

		printf("Connected: %s (%s)\n", inet_ntoa(cli_addr.sin_addr), username);

		add_tail(newsockfd, username, roomNum); // add this new client to the client list

		// print client list if thread creation was successful
		// print_client_list();

		// prepare ThreadArgs structure to pass client socket
		ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
		if (args == NULL) error("ERROR creating thread argument");
		
		args->clisockfd = newsockfd;
		
		pthread_t tid;
		if (pthread_create(&tid, NULL, thread_main, (void*) args) != 0) error("ERROR creating a new thread");
		else print_client_list();
	}

	close(sockfd);

	return 0; 
}

