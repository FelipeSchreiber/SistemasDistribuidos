//Trabalho prático 1 de Sistemas Distribuídos - Ana Paula Falcão e Felipe Schreiber

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MSGLEN 64

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(2);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
       cout << "Not enough arguments" << endl;
       cout << "Usage: " << argv[0] << " hostname port" << endl;
       exit(1);
    }
    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        error("ERROR no such host");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    else {
        char message[MSGLEN];
        bool consumer = true;
        int pnum, prime;
        
        while(consumer) {
            bzero(message, sizeof(message));

            n = read(sockfd, message, sizeof(message));
            if (n < 0)
                error("ERROR reading from socket");

            pnum = atoi(message);
            if(pnum == 0)
                consumer = false;
            else {
                // begins checking if number is prime
                for(int i = 2; i < pnum; i++) { 
                    if (pnum % i == 0) {
                    	 prime=0;
                        break;
                    }
                    else if (i == pnum - 1)
                    	prime=1;
                   
                }
                sprintf (message, "%d", prime);
                n = write(sockfd, message, sizeof(message));
                if (n < 0)
                    error("ERROR writing to socket");
            }
        }
    }
    
    close(sockfd);
    return 0;
}
