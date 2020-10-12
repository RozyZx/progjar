#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#define listenQ 5
#define MAXLINE 1024

typedef struct sockaddr SA;

int main(int argc, char **argv)
{
    int     sockfd, n, index = 0;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char    recvline[MAXLINE + 1], c, word[1024];
    char *addr;

    if(argc == 1) addr = "127.0.0.1";
    else addr = argv[1];

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(2020);
    if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <=0 ) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) == -1) {
      perror("connect");
      exit(EXIT_FAILURE);
    }

    //looping input
    for(;;){
      index = 0;
      memset(word, 0, sizeof(word));
      do{
        c = (char)fgetc(stdin);
        word[index] = c;
        index++;
      } while(c != '\n');

      if(write(sockfd, word, strlen(word)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
      }
      char recv[1024];
      memset(recv, 0, sizeof(recv));
      if(read(sockfd, recv, sizeof(recv)) < 0) {
        perror("read");
        exit(4);
      }
      printf("%s", recv);
    }
    close(sockfd);
}
