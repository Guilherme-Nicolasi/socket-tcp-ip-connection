#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#define MAX_CLIENTS 1000
#define MESSAGE_SIZE 1024

/*
* SERVER:
* 1. Create a socket;
* 2. Build the sockaddr_in;
* 3. Bind the socket to IP address and port;
* 4. Listen for connections;
* 5. Accept connections;
* 6. Create a new process for new connections;
* 7. Read/Send data;
* 8. Repeat 5, 6 and 7 in loop.
*/

typedef struct {
    int client_sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    pthread_t thread_id;
} Client;

size_t i = 0;
Client arr_clients[MAX_CLIENTS];

void BroadCast(const char *addr, const char *message, int sockfd) {
    char buffer[MESSAGE_SIZE];
    snprintf(buffer, MESSAGE_SIZE, "%s: %s\n", addr, message);
    for(size_t j = 0; j <= i; j++) {
        if(arr_clients[j].client_sockfd != sockfd) {
            ssize_t bytesWrite = write(arr_clients[j].client_sockfd, buffer, strlen(buffer));
            if(bytesWrite < 0) {
                std::cerr << "Error: Isn't possible sending the massage to client." << std::endl;
                break;
            }
        }
    }
}

void *Dispatch(void *args) {
    Client client = *((Client *)args);
    char buffer[MESSAGE_SIZE];

    while(true) {
        bzero(buffer, MESSAGE_SIZE);
        //fflush(stdout);

        ssize_t bytesRead = read(client.client_sockfd, buffer, (MESSAGE_SIZE - 1));
        if(bytesRead < 0) {
            std::cerr << "Error: Wasn't possible receiving the client's message." << std::endl;
            break;
        }

        if(strcmp(buffer, "\\exit\n") == 0) {
            BroadCast(inet_ntoa(client.client_addr.sin_addr), "Saiu.", client.client_sockfd);
            break;
        }

        buffer[bytesRead - 1] = '\0';
        BroadCast(inet_ntoa(client.client_addr.sin_addr), buffer, client.client_sockfd);
    }

    close(client.client_sockfd);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cerr << "Error: Required command-line: " << argv[0] << " <port>" << std::endl;
        return -1;
    }

    int server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server_sockfd == -1) {
        std::cerr << "Error: Wasn't possible opening a socket." << std::endl;
        return -1;
    }

    struct sockaddr_in serv_addr;
    bzero(reinterpret_cast<char *>(&serv_addr), sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_family = AF_INET;

    if(bind(server_sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) == -1) {
        std::cerr << "Error: Isn't possible biding the socket." << std::endl;
        close(server_sockfd);
        return -1;
    }

    if(listen(server_sockfd, MAX_CLIENTS) == -1) {
        std::cerr << "Error: Isn't possible listening." << std::endl;
        close(server_sockfd);
        return -1;
    }

    while(true) {
        arr_clients[i].addr_len = sizeof(arr_clients[i].client_addr);
        bzero(reinterpret_cast<char *>(&arr_clients[i].client_addr), arr_clients[i].addr_len);

        arr_clients[i].client_sockfd = accept(server_sockfd, reinterpret_cast<struct sockaddr *>(&arr_clients[i].client_addr), reinterpret_cast<socklen_t *>(&arr_clients[i].addr_len));
        if(arr_clients[i].client_sockfd == -1) {
            std::cerr << "Error: Wasn't possible accepting the connection." << std::endl;
            close(arr_clients[i].client_sockfd);
            break;
        }

        BroadCast(inet_ntoa(arr_clients[i].client_addr.sin_addr), "Cheguei!", arr_clients[i].client_sockfd);

        if(pthread_create(&arr_clients[i].thread_id, NULL, Dispatch, (void *)&arr_clients[i]) != 0) {
            std::cerr << "Error: Wasn't possible creating the thread to receive server's message." << std::endl;
            close(arr_clients[i].client_sockfd);
            break;
        }

        i = ((i + 1) % MAX_CLIENTS);
    }

    close(server_sockfd);
    return 0;
}
