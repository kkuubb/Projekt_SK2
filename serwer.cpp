#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <vector>
#include <unordered_map>
#include <stdlib.h>
#include <algorithm>
#include <signal.h>

//bool wlaczony = false;
struct sockaddr_in serv;
int fd;
int conn;
char message[100] = "";
struct addrinfo address, *res, *p;

void inicjalizacjaSerwera(char* ipAddres, char* port)
{
    serv.sin_family = AF_INET;
    serv.sin_port = htons((uint16_t)port);
    serv.sin_addr.s_addr = INADDR_ANY;
    if (fd = socket(AF_INET, SOCK_STREAM, 0) > 0)
    {
        puts("Cos poszlo nie tak");
    }
    bind(fd, (struct sockaddr *)&serv, sizeof(serv));
    listen(fd, 5);
    while(conn = accept(fd, (struct sockaddr *)NULL, NULL))
    {
        int pid;
        if((pid = fork()) == 0)
            while (recv(conn, message, 100, 0)>0) 
            {
            printf("Message Received: %s\n", message);
            char message[100] = "";
            }
            exit(0);
    }

}

int main(int argc, char* argv[])
{
    inicjalizacjaSerwera(argv[1], argv[2]);

}