#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

// Include GPIO control
#include <gpiorasp2.h>

#define CONNMAX 1000
#define BYTES 1024

// Define the pin number for each output
// the L at the begining means light and the D door
// B stads for button
#define LLIVINGROOM 5
#define LDINNIGROOM 6
#define LKITCHEN 13
#define LMASTERBEDROOM 19
#define LBEDROOM 26
#define BLIVINGROOM 25
#define BDINIGROOM 8
#define BKITCHEN 7
#define BMASTERBEDROOM 9
#define BBEDROOM 11
#define DFRONT 2
#define DBACK 3
#define DBEDROOM 4
#define DMASTERBEDROOM 14

// State variables
int llrstate = 0;
int ldrstate = 0;
int lkstate = 0;
int lmbrstate = 0;
int lbrstate = 0;
int dbstate = 0;
int dfstate = 0;
int dbrstate = 0;
int dmbrstate = 0;


char *ROOT;
int listenfd, clients[CONNMAX];
void error(char *);
void startServer(char *);
void respond(int);
void execute(int);

int main(int argc, char* argv[])
{
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;    
	
	//Default Values PATH = ~/ and PORT=10000
	char PORT[6];
	ROOT = getenv("PWD");
	strcpy(PORT,"10000");

	int slot=0;

	// Enable pins
	GPIOExport(LLIVINGROOM);
	GPIOExport(LDINNIGROOM);
	GPIOExport(LKITCHEN);
	GPIOExport(LMASTERBEDROOM);
	GPIOExport(LBEDROOM);
	GPIOExport(BLIVINGROOM);
	GPIOExport(BDINIGROOM);
	GPIOExport(BKITCHEN);
	GPIOExport(BMASTERBEDROOM);
	GPIOExport(BBEDROOM);
	GPIOExport(DFRONT);
	GPIOExport(DBEDROOM);
	GPIOExport(DMASTERBEDROOM);
	GPIOExport(DBACK);

	// Set GPIO mode
	pinMode(LLIVINGROOM, OUT);
	pinMode(LDINNIGROOM, OUT);
	pinMode(LKITCHEN, OUT);
	pinMode(LMASTERBEDROOM, OUT);
	pinMode(LBEDROOM, OUT);
	pinMode(BLIVINGROOM, IN);
	pinMode(BDINIGROOM, IN);
	pinMode(BKITCHEN, IN);
	pinMode(BMASTERBEDROOM, IN);
	pinMode(BBEDROOM, IN);
	pinMode(DFRONT, IN);
	pinMode(DBEDROOM, IN);
	pinMode(DMASTERBEDROOM, IN);
	pinMode(DBACK, IN);
	



	//Parsing the command line arguments
	while ((c = getopt (argc, argv, "p:r:")) != -1)
		switch (c)
		{
			case 'r':
				ROOT = malloc(strlen(optarg));
				strcpy(ROOT,optarg);
				break;
			case 'p':
				strcpy(PORT,optarg);
				break;
			case '?':
				fprintf(stderr,"Wrong arguments given!!!\n");
				exit(1);
			default:
				exit(1);
		}
	
	printf("Server started at port no. %s%s%s with root directory as %s%s%s\n","\033[92m",PORT,"\033[0m","\033[92m",ROOT,"\033[0m");
	// Setting all elements to -1: signifies there is no client connected
	int i;
	for (i=0; i<CONNMAX; i++)
		clients[i]=-1;
	startServer(PORT);

	// ACCEPT connections
	while (1)
	{
		addrlen = sizeof(clientaddr);
		clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0)
			error ("accept() error");
		else
		{
			if ( fork()==0 )
			{
				respond(slot);
				exit(0);
			}
		}

		while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;

		// Read signals
		int readfd = digitalRead(DFRONT);
		int readb = digitalRead(DBACK);
		int readmbrd = digitalRead(DMASTERBEDROOM);
		int readbd = digitalRead(DBEDROOM);
		int readldr = digitalRead(LDINNIGROOM);
		int readllr = digitalRead(LLIVINGROOM);
		int readlk = digitalRead(LKITCHEN);
		int readlmbr = digitalRead(LMASTERBEDROOM);
		int readlbr = digitalRead(LBEDROOM);

		// Change the dinning room light
		if (readldr != 0){
			ldrstate = !ldrstate;
			digitalWrite(LDINNIGROOM, ldrstate);
		}
		// Change the living room light
		if (readllr != 0){
			llrstate = !llrstate;
			digitalWrite(LLIVINGROOM, llrstate);
		}
		// Change the kitchen light
		if (readlk != 0){
			lkstate = !lkstate;
			digitalWrite(LKITCHEN, lkstate);
		}
		// Change the bedroom light
		if (readlbr != 0){
			lbrstate = !lbrstate;
			digitalWrite(LBEDROOM, lbrstate);
		}
		// Change the master bedroom light
		if (readlmbr != 0){
			lmbrstate = !lmbrstate;
			digitalWrite(LMASTERBEDROOM, lmbrstate);
		}

		

	}

	return 0;
}

//start server
void startServer(char *port)
{
	struct addrinfo hints, *res, *p;

	// getaddrinfo for host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0)
	{
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next)
	{
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL)
	{
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if ( listen (listenfd, 1000000) != 0 )
	{
		perror("listen() error");
		exit(1);
	}
}

//client connection
void respond(int n)
{
	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
	int rcvd, fd, bytes_read;

	memset( (void*)mesg, (int)'\0', 99999 );

	rcvd=recv(clients[n], mesg, 99999, 0);

	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
		printf("%s", mesg);
		execute((int) mesg);
		reqline[0] = strtok (mesg, " \t\n");
		if ( strncmp(reqline[0], "GET\0", 4)==0 )
		{
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");
			if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
				write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
			}
			else
			{
				if ( strncmp(reqline[1], "/\0", 2)==0 )
					reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

				strcpy(path, ROOT);
				strcpy(&path[strlen(ROOT)], reqline[1]);
				printf("file: %s\n", path);

				if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
				{
					send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
						write (clients[n], data_to_send, bytes_read);
				}
				else    write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
			}
		}
	}

	//Closing SOCKET
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
}






/**
 * Recive the msg from the server and execute the desired action
 * command: Action to execute
 */
void execute(int command){
	
	switch (command)
	{
	case 0:
		// Write on bedroom light
		lbrstate = !lbrstate;
		digitalWrite(LBEDROOM, lbrstate);
		break;
	case 1:
		// Write on master bedroom light
		lmbrstate = !lmbrstate;
		digitalWrite(LMASTERBEDROOM, lmbrstate);
		break;
	case 2:
		// Write on kitchen light
		lkstate = !lkstate;
		digitalWrite(LKITCHEN, lkstate);
		break;
	case 3:
		// Write on dining room light
		ldrstate = !ldrstate;
		digitalWrite(LDINNIGROOM, ldrstate);
		break;
	case 4:
		// Write on living room light
		llrstate = !llrstate;
		digitalWrite(LLIVINGROOM, llrstate);
		break;
	case 5:
		// Negate the state of the back door
		dbstate = !dbstate;
		break;
	case 6:
		// Negate the state of the front door
		dfstate = !dfstate;
		break;
	case 7:
		// Negate the state of the bedroom door
		dbrstate = !dbrstate;
		break;
	case 8:
		// Negate the state of the door
		dmbrstate = !dmbrstate;
		break;
	
	default:
		break;
	}

}


























