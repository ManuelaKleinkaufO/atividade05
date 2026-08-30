#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(void) {

    int clientSocket;

    struct sockaddr_in serverAddress;


    // Criação do socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0) {
        perror("Erro ao criar socket");
        return EXIT_FAILURE;
    }


    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);


    // Converte o endereço IP para o formato utilizado pelo socket
    if (inet_pton(
            AF_INET,
            "127.0.0.1",
            &serverAddress.sin_addr) <= 0) {

        perror("Endereco IP invalido");
        close(clientSocket);

        return EXIT_FAILURE;
    }


    printf("Conectando ao servidor...\n");


    // Conecta ao servidor
    if (connect(
            clientSocket,
            (struct sockaddr*)&serverAddress,
            sizeof(serverAddress)) < 0) {

        perror("Erro ao conectar ao servidor");
        close(clientSocket);

        return EXIT_FAILURE;
    }


    printf("Conectado ao servidor.\n");


    char request[] = "Requisicao do cliente";


    // Envia a requisição
    if (send(
            clientSocket,
            request,
            strlen(request),
            0) < 0) {

        perror("Erro ao enviar requisicao");

        close(clientSocket);

        return EXIT_FAILURE;
    }


    printf("Requisicao enviada: %s\n", request);


    char buffer[BUFFER_SIZE];

    memset(buffer, 0, sizeof(buffer));


    // Aguarda resposta do servidor
    ssize_t bytesRecebidos = recv(
        clientSocket,
        buffer,
        sizeof(buffer) - 1,
        0
    );


    if (bytesRecebidos < 0) {

        perror("Erro ao receber resposta");

        close(clientSocket);

        return EXIT_FAILURE;
    }


    if (bytesRecebidos == 0) {

        printf("O servidor encerrou a conexao sem enviar resposta.\n");

        close(clientSocket);

        return EXIT_FAILURE;
    }


    // Finaliza corretamente a string
    buffer[bytesRecebidos] = '\0';


    printf("\n========================================\n");
    printf(" Resposta do servidor\n");
    printf("========================================\n");
    printf("%s\n", buffer);
    printf("========================================\n");


    close(clientSocket);

    return EXIT_SUCCESS;
}
