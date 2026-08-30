#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Estrutura usada para retornar o resultado da leitura de A.txt
typedef struct {
    long total_caracteres;
    int sucesso;
} ResultadoLeitura;

// Estrutura usada para informar se B.txt foi escrito corretamente
typedef struct {
    int sucesso;
} ResultadoEscrita;


// Thread responsável por ler o arquivo A.txt
void* ler_arquivo_A(void* arg) {

    ResultadoLeitura* resultado = (ResultadoLeitura*)arg;

    resultado->total_caracteres = 0;
    resultado->sucesso = 0;

    FILE* arquivo = fopen("A.txt", "r");

    if (arquivo == NULL) {
        perror("Erro ao abrir A.txt");
        pthread_exit(NULL);
    }

    int caractere;

    // Conta os caracteres do arquivo
    while ((caractere = fgetc(arquivo)) != EOF) {
        resultado->total_caracteres++;
    }

    // Verifica se ocorreu algum erro durante a leitura
    if (ferror(arquivo)) {
        perror("Erro ao ler A.txt");
        fclose(arquivo);
        pthread_exit(NULL);
    }

    fclose(arquivo);

    resultado->sucesso = 1;

    pthread_exit(NULL);
}


// Thread responsável por escrever no arquivo B.txt
void* escrever_arquivo_B(void* arg) {

    ResultadoEscrita* resultado = (ResultadoEscrita*)arg;

    resultado->sucesso = 0;

    FILE* arquivo = fopen("B.txt", "a");

    if (arquivo == NULL) {
        perror("Erro ao abrir B.txt");
        pthread_exit(NULL);
    }

    pid_t pid = getpid();

    time_t agora = time(NULL);

    if (agora == (time_t)-1) {
        perror("Erro ao obter data e hora");
        fclose(arquivo);
        pthread_exit(NULL);
    }

    struct tm horario;

    // localtime_r é uma versão segura para uso com threads
    if (localtime_r(&agora, &horario) == NULL) {
        fprintf(stderr, "Erro ao converter data e hora.\n");
        fclose(arquivo);
        pthread_exit(NULL);
    }

    int retorno = fprintf(
        arquivo,
        "PID: %d | Data/Hora: %04d-%02d-%02d %02d:%02d:%02d\n",
        (int)pid,
        horario.tm_year + 1900,
        horario.tm_mon + 1,
        horario.tm_mday,
        horario.tm_hour,
        horario.tm_min,
        horario.tm_sec
    );

    if (retorno < 0) {
        perror("Erro ao escrever em B.txt");
        fclose(arquivo);
        pthread_exit(NULL);
    }

    if (fclose(arquivo) != 0) {
        perror("Erro ao fechar B.txt");
        pthread_exit(NULL);
    }

    resultado->sucesso = 1;

    pthread_exit(NULL);
}


int main(void) {

    int serverSocket;
    int clientSocket;

    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    socklen_t clientAddressLength = sizeof(clientAddress);

    int opt = 1;


    // Criação do socket do servidor
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        perror("Erro ao criar socket do servidor");
        return EXIT_FAILURE;
    }


    // Permite reutilizar a porta após reiniciar o servidor
    if (setsockopt(
            serverSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)) < 0) {

        perror("Erro no setsockopt");
        close(serverSocket);
        return EXIT_FAILURE;
    }


    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);


    // Associação do socket à porta
    if (bind(
            serverSocket,
            (struct sockaddr*)&serverAddress,
            sizeof(serverAddress)) < 0) {

        perror("Erro ao associar socket à porta");
        close(serverSocket);
        return EXIT_FAILURE;
    }


    // Coloca o servidor em modo de escuta
    if (listen(serverSocket, 5) < 0) {

        perror("Erro no listen");
        close(serverSocket);
        return EXIT_FAILURE;
    }


    printf("========================================\n");
    printf(" Servidor iniciado\n");
    printf(" Porta: %d\n", PORT);
    printf("========================================\n");


    while (1) {

        printf("\nAguardando uma conexão...\n");

        clientSocket = accept(
            serverSocket,
            (struct sockaddr*)&clientAddress,
            &clientAddressLength
        );

        if (clientSocket < 0) {
            perror("Erro ao aceitar conexão");
            continue;
        }


        printf(
            "Cliente conectado: %s:%d\n",
            inet_ntoa(clientAddress.sin_addr),
            ntohs(clientAddress.sin_port)
        );


        char buffer[BUFFER_SIZE];

        memset(buffer, 0, sizeof(buffer));


        // Recebe a requisição do cliente
        ssize_t bytesRecebidos = recv(
            clientSocket,
            buffer,
            sizeof(buffer) - 1,
            0
        );


        if (bytesRecebidos <= 0) {

            if (bytesRecebidos < 0) {
                perror("Erro ao receber requisição");
            } else {
                printf("Cliente encerrou a conexão.\n");
            }

            close(clientSocket);
            continue;
        }


        // Garante que a mensagem termine corretamente
        buffer[bytesRecebidos] = '\0';

        printf("Requisição recebida: %s\n", buffer);


        pthread_t threadLeitura;
        pthread_t threadEscrita;

        ResultadoLeitura resultadoLeitura;
        ResultadoEscrita resultadoEscrita;

        resultadoLeitura.total_caracteres = 0;
        resultadoLeitura.sucesso = 0;

        resultadoEscrita.sucesso = 0;


        // Criação da primeira thread
        int erroThread1 = pthread_create(
            &threadLeitura,
            NULL,
            ler_arquivo_A,
            &resultadoLeitura
        );


        if (erroThread1 != 0) {

            fprintf(
                stderr,
                "Erro ao criar thread de leitura: %s\n",
                strerror(erroThread1)
            );

            const char* mensagem =
                "Erro interno: nao foi possivel criar a thread de leitura.";

            send(clientSocket, mensagem, strlen(mensagem), 0);

            close(clientSocket);

            continue;
        }


        // Criação da segunda thread
        int erroThread2 = pthread_create(
            &threadEscrita,
            NULL,
            escrever_arquivo_B,
            &resultadoEscrita
        );


        if (erroThread2 != 0) {

            fprintf(
                stderr,
                "Erro ao criar thread de escrita: %s\n",
                strerror(erroThread2)
            );

            // A primeira thread já foi criada, então esperamos sua conclusão
            pthread_join(threadLeitura, NULL);

            const char* mensagem =
                "Erro interno: nao foi possivel criar a thread de escrita.";

            send(clientSocket, mensagem, strlen(mensagem), 0);

            close(clientSocket);

            continue;
        }


        // Aguarda as duas threads terminarem
        pthread_join(threadLeitura, NULL);
        pthread_join(threadEscrita, NULL);


        char resposta[256];


        // Somente envia sucesso caso as duas tarefas tenham sido concluídas
        if (resultadoLeitura.sucesso &&
            resultadoEscrita.sucesso) {

            snprintf(
                resposta,
                sizeof(resposta),
                "Sucesso! O arquivo A.txt possui %ld caracteres.",
                resultadoLeitura.total_caracteres
            );

            printf(
                "Processamento concluido. A.txt possui %ld caracteres.\n",
                resultadoLeitura.total_caracteres
            );

        } else {

            snprintf(
                resposta,
                sizeof(resposta),
                "Erro ao processar a requisicao."
            );

            printf("Ocorreu um erro durante o processamento.\n");
        }


        // Envia resposta ao cliente
        if (send(
                clientSocket,
                resposta,
                strlen(resposta),
                0) < 0) {

            perror("Erro ao enviar resposta ao cliente");
        }


        close(clientSocket);

        printf("Conexao com o cliente encerrada.\n");
    }


    close(serverSocket);

    return EXIT_SUCCESS;
}
