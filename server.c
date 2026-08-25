#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080

void* ler_arquivo_A(void* arg) {
    long* total_caracteres = (long*)arg;
    FILE* file = fopen("A.txt", "r");
    if (!file) {
        perror("Erro ao abrir A.txt");
        *total_caracteres = -1;
        pthread_exit(NULL);
    }

    fseek(file, 0, SEEK_END);
    *total_caracteres = ftell(file);
    fclose(file);

    pthread_exit(NULL);
}

void* escrever_arquivo_B(void* arg) {
    FILE* file = fopen("B.txt", "a");
    if (!file) {
        perror("Erro ao abrir B.txt");
        pthread_exit(NULL);
    }

    pid_t pid = getpid();
    time_t agora = time(NULL);
    struct tm *t = localtime(&agora);

    fprintf(file, "PID: %d | Data/Hora: %04d-%02d-%02d %02d:%02d:%02d\n",
            pid, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    fclose(file);
    pthread_exit(NULL);
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress;
    int opt = 1;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Erro ao criar socket");
        return 1;
    }

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Erro ao associar o socket");
        return 1;
    }

    listen(serverSocket, 5);
    printf("Servidor rodando na porta %d...\n", PORT);

    while (1) {
        clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket < 0) continue;

        char buffer[1024] = {0};
        recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        pthread_t t1, t2;
        long qtd_caracteres = 0;

        pthread_create(&t1, NULL, ler_arquivo_A, &qtd_caracteres);
        pthread_create(&t2, NULL, escrever_arquivo_B, NULL);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);

        char response[256];
        snprintf(response, sizeof(response), "Sucesso! O arquivo A.txt possui %ld caracteres.", qtd_caracteres);
        send(clientSocket, response, strlen(response), 0);

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}