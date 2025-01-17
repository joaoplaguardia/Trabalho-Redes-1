#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>

#define MAX_QUEUE_SIZE 10
#define NUM_THREADS 4
#define FILENAME "arquivos/teste.bin"

typedef struct {
    int sockets[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} task_queue_t;

void init_queue(task_queue_t *queue) {
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

void enqueue(task_queue_t *queue, int socket) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->count == MAX_QUEUE_SIZE) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    queue->sockets[queue->rear] = socket;
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->count++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

int dequeue(task_queue_t *queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    int socket = queue->sockets[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->count--;
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return socket;
}

void handle_client(int client_socket) {
    char buffer[1024];
    read(client_socket, buffer, sizeof(buffer) - 1);

    printf("Requisição recebida:\n%s\n", buffer);

    if (strncmp(buffer, "GET", 3) == 0) {
        int file_fd = open(FILENAME, O_RDONLY);
        if (file_fd < 0) {
            perror("Erro ao abrir arquivo");
            const char *error_response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            write(client_socket, error_response, strlen(error_response));
            close(client_socket);
            return;
        }

        off_t file_size = lseek(file_fd, 0, SEEK_END);
        lseek(file_fd, 0, SEEK_SET);

        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %ld\r\n"
                 "Content-Type: application/octet-stream\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 file_size);
        write(client_socket, header, strlen(header));

        char file_buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_socket, file_buffer, bytes_read);
        }

        close(file_fd);
        printf("Arquivo enviado com sucesso.\n");
    } else {
        const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        write(client_socket, response, strlen(response));
    }

    close(client_socket);
}

void *worker_thread(void *arg) {
    task_queue_t *queue = (task_queue_t *)arg;
    while (1) {
        int client_socket = dequeue(queue);
        handle_client(client_socket);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(1);
    }

    int portno = atoi(argv[1]);
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro no bind");
        close(server_socket);
        exit(1);
    }

    listen(server_socket, 5);
    printf("Servidor escutando na porta %d...\n", portno);

    task_queue_t queue;
    init_queue(&queue);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, &queue);
    }

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &clilen);
        if (client_socket < 0) {
            perror("Erro no accept");
            continue;
        }

        enqueue(&queue, client_socket);
    }

    close(server_socket);
    return 0;
}
