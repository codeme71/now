// now -- internet manager for unix system

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char  * argv[]) {
    if (argc < 3) {
        printf("Usage: %s <url> <output file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[2], O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    char  * url = argv[1];
    char  * filename = argv[2];

    int response = 0;
    FILE  * file = fopen(filename, "w");
    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    while ((response = get_http_response(url, file)) == 0) {
        printf("Downloading...\n");
    }

    if (response < 0) {
        perror("get_http_response");
        return 1;
    }

    fclose(file);
    close(fd);
    return 0;
}

int get_http_response(char  * url, FILE  * file) {
    struct addrinfo hints,  * servinfo,  * p;
    int sockfd, numbytes;
    char  * host,  * path;
    char buffer[1024];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    // Разделяем URL на хост и путь
    host = strtok(url, "/");
    path = strtok(NULL, "/");

    // Инициализируем структуру hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Получаем информацию о хосте
    if ((getaddrinfo(host, "http", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errno));
        return -1;
    }

    // Открываем сокет
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        // Соединяемся с сервером
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Could not connect.\n");
        return -1;
    }

    // Отправляем GET-запрос
    snprintf(buffer, sizeof(buffer), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    if (write(sockfd, buffer, strlen(buffer)) == -1) {
        perror("write");
        close(sockfd);
        return -1;
    }

    freeaddrinfo(servinfo);

    // Читаем ответ сервера
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0, (struct sockaddr  * )&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        close(sockfd);
        return -1;
    }

    buffer[numbytes] = '\0';
    fprintf(file, "%s", buffer);

    close(sockfd);
    return 0;
}


