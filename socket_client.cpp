#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#define MESSAGE_SIZE 1024

/*
* CLIENT:
* 1. Create a socket;
* 2. Build the sockaddr_in;
* 3. Connect to the server;
* 4. Send data;
* 5. Receive a reply;
* 6. Repeat 4 and 5 in loop.
*/

void *Read(void *arg) {
    int sockfd = *((int *)arg);
    char buffer[MESSAGE_SIZE];
    ssize_t bytesRead;

    while(true) {
        bzero(buffer, MESSAGE_SIZE);
        bytesRead = read(sockfd, buffer, (MESSAGE_SIZE - 1));
        if(bytesRead < 0) {
            std::cerr << "Error: Wasn't possible receiving the server's messages." << std::endl;
            break;
        }

        std::cout << buffer;

        if(buffer[strlen(buffer) - 1] != '\n') {
            std::cout << std::endl;
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if(argc != 3) {
        std::cerr << "Error: Required command-line: " << argv[0] << " <server_addr> <port>" << std::endl;
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        std::cerr << "Error: Wasn't possible opening a socket." << std::endl;
        return -1;
    }

    struct sockaddr_in serv_addr;
    bzero(reinterpret_cast<char *>(&serv_addr), sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    /*struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error: Wasn't possible receiving the timeout." << std::endl;
        close(sockfd);
        return -1;
    }*/

    if(connect(sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0) {
        std::cerr << "Error: Isn't possible connecting to server." << std::endl;
        close(sockfd);
        return -1;
    }

    pthread_t read_thread;
    if(pthread_create(&read_thread, NULL, Read, &sockfd) != 0) {
        std::cerr << "Error: Wasn't possible creating the thread to receive server's message." << std::endl;
        close(sockfd);
        return -1;
    }

    char buffer[MESSAGE_SIZE];
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    getsockname(sockfd, (struct sockaddr *)&local_addr, &addr_len);

    while(true) {
        //printf("%s: ", inet_ntoa(local_addr.sin_addr));
        //fflush(stdout);

        bzero(buffer, MESSAGE_SIZE);
        fgets(buffer, (MESSAGE_SIZE - 1), stdin);

        ssize_t bytesWrite = write(sockfd, buffer, strlen(buffer));
        if(bytesWrite < 0) {
            std::cerr << "Error: Isn't possible sending the massage to server." << std::endl;
            break;
        }

        if(strcmp(buffer, "\\exit\n") == 0) {
            break;
        }
    }

    close(sockfd);
    return 0;
}
