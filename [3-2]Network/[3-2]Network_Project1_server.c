
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>

#define BUFFER_SIZE 4096

void send_file_to_client(int client_socket, const char* file_name) {
    FILE* file = fopen(file_name, "rb");
    if (!file) {
        perror("파일 오픈 에러");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (send(client_socket, &file_size, sizeof(long), 0) < 0) {
        perror("파일 크기 전송 에러");
        fclose(file);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            perror("파일 전송 에러");
            fclose(file);
            return;
        }
    }

    fclose(file);
}

void receive_file_from_client(int client_socket, const char* file_name) {
    long file_size;
    if (recv(client_socket, &file_size, sizeof(long), 0) < 0) {
        perror("파일 크기 수신 에러");
        return;
    }

    FILE* file = fopen(file_name, "wb");
    if (!file) {
        perror("파일 오픈 에러");
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_received;
    long total_received = 0;

    while (total_received < file_size) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("연결이 끊어졌습니다.\n");
            } else {
                perror("파일 수신 에러");
            }
            break;
        }

        size_t bytes_written = fwrite(buffer, 1, bytes_received, file);
        if (bytes_written < bytes_received) {
            perror("파일 쓰기 에러");
            fclose(file);
            return;
        }

        total_received += bytes_received;
    }

    fclose(file);
}

void *client_handler(void *arg) {
    int client_socket = *((int *)arg);

    char buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

    if (strcmp(buffer, "1\n") == 0) {
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(".")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_REG) {
                    write(client_socket, ent->d_name, strlen(ent->d_name));
                    write(client_socket, "\n", 1);
                }
            }
            closedir(dir);
        } else {
            perror("디렉터리 열기 에러");
        }
    } else {
        char file_name[BUFFER_SIZE];
        memset(file_name, 0, BUFFER_SIZE);
        recv(client_socket, file_name, BUFFER_SIZE - 1, 0);
        receive_file_from_client(client_socket, file_name);
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("소켓 생성 에러");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(9739); 

    if (bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("바인딩 에러");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("대기 에러");
        exit(EXIT_FAILURE);
    }

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &cli_addr_len);
        if (new_socket < 0) {
            perror("연결 수락 에러");
            exit(EXIT_FAILURE);
        }

        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("20190739 박경태, 파일 송수신 서버입니다.\n\n");
        printf("ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ\n");
        printf("연결된 클라이언트 정보\n");
        printf("클라이언트 IP 주소: %s\n", client_ip);
        printf("클라이언트 포트 번호: %d\n", ntohs(cli_addr.sin_port));

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, (void *)&new_socket) != 0) {
            perror("스레드 생성 에러");
            close(new_socket);
        }

        pthread_detach(thread);
    }

    close(server_socket);
    return 0;
}
