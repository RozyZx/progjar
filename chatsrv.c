#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>

#define LISTEN_PORT 2020
#define MESSAGE_BUFFER_SIZE 512
#define BROADCAST_BUFFER_SIZE 1024
#define CONNECTION_QUEUE 5
#define MAX_FILE_DESCRIPTOR 30

void cerror(char* errorSource) {
    perror(errorSource);
    exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {
    // init variables
    int listenerFileDescriptor,
        incomingClientFileDescriptor,
        bytesRead,
        bytesSent;
    socklen_t clientAddressLength;
    struct sockaddr_in  serverAddress,
                        clientAddress;
    char    clientAddressString[INET_ADDRSTRLEN],
            clientMessageBuffer[MESSAGE_BUFFER_SIZE],
            broadcastMessageBuffer[BROADCAST_BUFFER_SIZE];
    fd_set  activeFileDescriptors,
            readyFileDescriptors;

    // socket
    listenerFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    // init FD
    FD_ZERO(&activeFileDescriptors); // fill with zeros
    FD_SET(listenerFileDescriptor, &activeFileDescriptors); // add the server listening socket to the file descriptor set

    // init serv addr (IP, port, IP type)
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htons(INADDR_ANY);
    serverAddress.sin_port = htons(LISTEN_PORT);

    // bind the socket to server address
    if (bind(listenerFileDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
        cerror("bind");

    // listening
    if (listen(listenerFileDescriptor, CONNECTION_QUEUE) < 0)
        cerror("listen");

    while(1) {
        readyFileDescriptors = activeFileDescriptors;

        if (select(MAX_FILE_DESCRIPTOR, &readyFileDescriptors, NULL, NULL, NULL) < 0)
            cerror("select");

        for (int currentFileDescriptor = 3; currentFileDescriptor < MAX_FILE_DESCRIPTOR; currentFileDescriptor++) {
            if (FD_ISSET(currentFileDescriptor, &readyFileDescriptors)) {
                if (currentFileDescriptor == listenerFileDescriptor) {
                    clientAddressLength = sizeof(clientAddress);
                    incomingClientFileDescriptor = accept(currentFileDescriptor, (struct sockaddr *) &clientAddress, &clientAddressLength);
                    if (incomingClientFileDescriptor < 0) cerror("accept");
                    FD_SET(incomingClientFileDescriptor, &activeFileDescriptors);
                    inet_ntop(AF_INET, &clientAddress.sin_addr, clientAddressString, INET_ADDRSTRLEN);
                    if(argc > 1) printf("[SERVER] New connection from %s:%d will be handled by FD %d\n", clientAddressString, clientAddress.sin_port, incomingClientFileDescriptor);
                }
                else {
                    incomingClientFileDescriptor = currentFileDescriptor;
                    bzero(clientMessageBuffer, sizeof(clientMessageBuffer));
                    bytesRead = read(incomingClientFileDescriptor, clientMessageBuffer, MESSAGE_BUFFER_SIZE);
                    if (bytesRead < 0) cerror("read");
                    else if (bytesRead == 0) {
                        if(argc > 1) printf("[SERVER] Done reading from FD %d, or the client terminated. Connection in this socket will be closed\n", incomingClientFileDescriptor);
                        close(incomingClientFileDescriptor);
                        FD_CLR(currentFileDescriptor, &activeFileDescriptors);
                    } else {
                        inet_ntop(AF_INET, &clientAddress.sin_addr, clientAddressString, INET_ADDRSTRLEN);
                        if(argc > 1) printf("[MESSAGE] %s -> %s", clientAddressString, clientMessageBuffer);
                        sprintf(broadcastMessageBuffer, "%s -> %s", clientAddressString, clientMessageBuffer);

                        for (int broadcastFileDescriptor = 3; broadcastFileDescriptor < MAX_FILE_DESCRIPTOR; broadcastFileDescriptor++) {
                            if (FD_ISSET(broadcastFileDescriptor, &activeFileDescriptors)) {
                                if (broadcastFileDescriptor != listenerFileDescriptor && broadcastFileDescriptor != incomingClientFileDescriptor) {
                                    bytesSent = write(broadcastFileDescriptor, broadcastMessageBuffer, sizeof(broadcastMessageBuffer));
                                    if (bytesSent < 0) cerror("write");
                                    if(argc > 1) printf("[SERVER] %d bytes sent to %s:%d via FD %d\n", bytesSent, clientAddressString, clientAddress.sin_port, broadcastFileDescriptor);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}