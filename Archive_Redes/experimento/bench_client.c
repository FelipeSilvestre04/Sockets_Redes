#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define DEFAULT_BUFFER_SIZE 65536

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Uso: %s <IP> <porta> <duracao_seg> [tam_bloco]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    double duration = atof(argv[3]);
    size_t buf_size = (argc == 5) ? (size_t)atoi(argv[4]) : DEFAULT_BUFFER_SIZE;

    signal(SIGPIPE, SIG_IGN);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Endereco IPv4 invalido.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    char *buffer = malloc(buf_size);
    if (!buffer) {
        perror("malloc");
        close(sockfd);
        return EXIT_FAILURE;
    }
    memset(buffer, 'A', buf_size); // Preenche o payload com dados fixos

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    unsigned long long total_sent = 0;

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - start.tv_sec) + 
                         (now.tv_nsec - start.tv_nsec) / 1e9;
        if (elapsed >= duration) {
            break;
        }

        ssize_t n = send(sockfd, buffer, buf_size, MSG_NOSIGNAL);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("send");
            break;
        } else if (n == 0) {
            break;
        }
        total_sent += (unsigned long long)n;
    }

    free(buffer);
    close(sockfd);
    return EXIT_SUCCESS;
}