#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Erro ao criar o socket");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(PORT);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Erro ao conectar ao servidor");
        return 1;
    }

    char request[] = "Requisição do cliente";
    send(clientSocket, request, strlen(request), 0);

    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    printf("Resposta do servidor: %s\n", buffer);

    close(clientSocket);
    return 0;
}