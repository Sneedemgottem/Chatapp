#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#define MAXMSGLEN 500
#define MAXUSERLEN 128
#define PORTNUM "2300"
#define BACKLOG 5

// Term colors
#define SET_RED printf("\033[0;31m")
#define SET_YELLOW printf("\033[0;33m")
#define RESET_COLOR printf("\033[0m")

extern int errno;

void start_serv(const char* port);
void join_serv(const char* port, const char* username);
void handle_incoming_socket(int incomingfd, fd_set* curr);

void handle_client_opt();
int check_error(int expr);

int main()
{
	SET_YELLOW;
	puts("Welcome to Sneeds Chatapp!");
	puts("Choose an option:");
	RESET_COLOR;
	puts("(1) Start the chatroom");
	puts("(2) Join the chatroom");
	SET_YELLOW;
	printf("Enter a number (1/2): ");
	RESET_COLOR;

	int choice;
	scanf("%d", &choice);

	switch (choice)
	{
		case 1: // Server option
			start_serv(PORTNUM);
			break;
		case 2: // Client option
			handle_client_opt();
			break;
		default:
			fprintf(stderr, "Invalid Option!\n");
			exit(EXIT_FAILURE);
	}

	return 0;
}

void handle_client_opt()
{
	char user[MAXUSERLEN];

	// Select Username
	SET_YELLOW;
	printf("Please enter a username: ");
	RESET_COLOR;

	scanf("%s", user);

	SET_YELLOW;
	printf("Joined as ");
	RESET_COLOR;
	printf("%s", user);
	SET_YELLOW;
	printf(", Enjoy!\n");
	RESET_COLOR;

	join_serv(PORTNUM, user);
}

void join_serv(const char* port, const char* username)
{
	struct addrinfo hints, *res; 
	int sockfd;

	// Get addrinfo of the server
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // Connect to localhost

	getaddrinfo(NULL, port, &hints, &res);
	sockfd = check_error(socket(res->ai_family, res->ai_socktype, res->ai_protocol)); // Create socket descriptor for client
	check_error(connect(sockfd, res->ai_addr, res->ai_addrlen)); // Connect socket to server

	freeaddrinfo(res);

	while (1)
	{
		char msg[MAXMSGLEN];
		fgets(msg, MAXMSGLEN, stdin);
		msg[strlen(msg) - 1] = '\0'; // Remove \n

		if (strlen(msg) == 0)
		{
			continue;
		}
		else
		{
			// Append username at the beginning of message
			char data[MAXMSGLEN + strlen(username) + 2];
			strcpy(data, username);
			strcat(data, ": ");
			strcat(data, msg);

			check_error(send(sockfd, data, strlen(data), 0));
		}
	}
}

void start_serv(const char* port)
{
	SET_YELLOW;
	printf("Starting server on port %s.\n", port);
	RESET_COLOR;

	struct addrinfo hints, *res;
	struct sockaddr_in6 their_addr;
	int sockfd;
	socklen_t addr_size;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // Working with IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, port, &hints, &res);

	sockfd = check_error(socket(res->ai_family, res->ai_socktype, res->ai_protocol)); // Create socket descriptor
	check_error(bind(sockfd, res->ai_addr, res->ai_addrlen)); // Bind socket to specified port
	check_error(listen(sockfd, BACKLOG)); // Listen on port

	addr_size = sizeof their_addr;
	
	// create sets	
	fd_set curr, ready;

	//init set
	FD_ZERO(&curr);
	FD_SET(sockfd, &curr);

	while (1)
	{
		ready = curr;
		check_error(select(FD_SETSIZE, &ready, NULL, NULL, NULL));

		for (int i = 0; i < FD_SETSIZE; i++)
		{
			if (FD_ISSET(i, &ready))
			{
				if (i == sockfd)
				{
					int incomingfd;
					incomingfd = check_error(accept(sockfd, (struct sockaddr*)&their_addr, &addr_size)); // Listening socket descriptor
					FD_SET(incomingfd, &curr);
					SET_YELLOW;
					puts("Someone has joined the room, say hello!");
					RESET_COLOR;
				}
				else
				{
					handle_incoming_socket(i, &curr);
				}
			}
		}


	}

	freeaddrinfo(res);
}

void handle_incoming_socket(int incomingfd, fd_set* curr)
{
	char buf[MAXMSGLEN];
	int len;

	len = check_error(recv(incomingfd, buf, MAXMSGLEN, 0));
	buf[len] = '\0';

	if (len == 0)
	{
		SET_YELLOW;
		puts("Client has disconnected.");
		RESET_COLOR;
		FD_CLR(incomingfd, curr);
		close(incomingfd);
	}
	else
	{
		printf("%s\n", buf);
	}

}

int check_error(int expr)
{
	if (expr < 0)
	{
		fprintf(stderr, "Error occured! Err num: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	return expr;
}
