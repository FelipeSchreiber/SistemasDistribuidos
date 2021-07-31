//Trabalho prático 1 de Sistemas Distribuídos - Ana Paula Falcão e Felipe Schreiber

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define MSGLEN 64

using namespace std;

int randomDelta(int i, int f){
  srand((unsigned)time(0));
  int bigger = f;
  int smaller = i;
  return rand()%(bigger-smaller+1) + smaller;
}

void error(const char *msg)
{
    perror(msg);
    exit(2);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     srand(time(NULL));

     if (argc < 3) {
         cout << "Not enough arguments" << endl;
         cout << "Usage: " << argv[0] << " port random_numbers" << endl;
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");

     else {
        char message[MSGLEN];
        bool producer = true;
        int count = atoi(argv[2]) + 1;
        int rnum = 3;
        int prime;
       
        while(producer){
            count--;
            bzero(message, sizeof(message));

            int delta = randomDelta(1, 100);

            rnum += delta + 1; 

            if (count <= 0) {
                producer = false;
            }

            sprintf (message, "%d", rnum);
            
            n = write(newsockfd, message, sizeof(message));
            if (n < 0)
                error("ERROR writing to socket");

            n = read(newsockfd, message, sizeof(message));
            if (n < 0)
                error("ERROR reading from socket");

            prime = atoi(message);
            if (prime == 1)
                cout<< "Prime number: "<< rnum << endl;
            if (prime == 0)
                cout<< "Not a Prime number: "<< rnum << endl;
        }
    }
     
     close(newsockfd);
     close(sockfd);
     return 0; 
}
