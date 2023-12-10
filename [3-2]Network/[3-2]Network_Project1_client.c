#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

void send_file(int client_socket, const char* file_name) {
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

void receive_file(int client_socket, const char* file_name) {
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

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    char* server_ip = argv[1];
    int server_port = atoi(argv[2]);

    struct sockaddr_in serv_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }

    printf("서버에 연결되었습니다.\n");

    int choice;
    printf("파일을 다운로드하려면 1을, 파일을 업로드하려면 2를 입력하세요: ");
    scanf("%d", &choice);

    if (choice == 1) {
        printf("파일 목록을 요청합니다...\n");
        send(client_socket, "1\n", strlen("1\n"), 0);

        char file_list[BUFFER_SIZE];
        int bytes_received;

        while ((bytes_received = recv(client_socket, file_list, BUFFER_SIZE - 1, 0)) > 0) {
            file_list[bytes_received] = '\0';
            printf("서버 디렉터리의 파일 목록:\n%s", file_list);
        }

        char file_to_download[BUFFER_SIZE];
        printf("다운로드할 파일 이름을 입력하세요: ");
        scanf("%s", file_to_download);

        send(client_socket, file_to_download, strlen(file_to_download), 0);
        printf("'%s' 파일을 다운로드합니다...\n", file_to_download);
        receive_file(client_socket, file_to_download);
        printf("'%s' 파일이 성공적으로 다운로드되었습니다.\n", file_to_download);
    } else if (choice == 2) {
        char file_to_upload[BUFFER_SIZE];
        printf("업로드할 파일 이름을 입력하세요: ");
        scanf("%s", file_to_upload);

        send(client_socket, "2\n", strlen("2\n"), 0);
        send(client_socket, file_to_upload, strlen(file_to_upload), 0);
        printf("'%s' 파일을 업로드합니다...\n", file_to_upload);
        send_file(client_socket, file_to_upload);
        printf("'%s' 파일이 성공적으로 업로드되었습니다.\n", file_to_upload);
    } else {
        printf("잘못된 선택입니다. 프로그램을 종료합니다.\n");
    }

    close(client_socket);
    return 0;
}
