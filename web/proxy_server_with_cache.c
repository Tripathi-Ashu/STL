#include "proxy_parse.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

LRUCache cache; // Shared cache

// Handle client requests
void* handle_client(void* arg) {
    int client_fd = (int)arg;

    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    printf("Client Request:\n%s\n", buffer);

    // Parse URL (simple example assumes GET requests)
    char url[256];
    sscanf(buffer, "GET %s HTTP/1.1", url);

    // Check cache
    char* cached_response = get_from_cache(&cache, url);
    if (cached_response) {
        printf("Cache hit for %s\n", url);
        write(client_fd, cached_response, strlen(cached_response));
    } else {
        printf("Cache miss for %s\n", url);

        // Forward request to target server
        int target_fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in target_addr;
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons(80);
        inet_pton(AF_INET, "93.184.216.34", &target_addr.sin_addr); // Example IP (www.example.com)

        if (connect(target_fd, (struct sockaddr*)&target_addr, sizeof(target_addr)) < 0) {
            perror("Connection to target server failed");
            close(client_fd);
            close(target_fd);
            return NULL;
        }

        write(target_fd, buffer, bytes_read);
        bytes_read = read(target_fd, buffer, BUFFER_SIZE - 1);
        buffer[bytes_read] = '\0';

        // Cache the response
        put_in_cache(&cache, url, buffer);

        // Send response to client
        write(client_fd, buffer, bytes_read);
        close(target_fd);
    }

    close(client_fd);
    return NULL;
}

int main() {
    init_cache(&cache);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    printf("Proxy server running on port %d...\n", PORT);

    while (1) {
        int* client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, NULL, NULL);
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_fd);
        pthread_detach(thread); // Detach thread to handle independently
    }

    free_cache(&cache);
    close(server_fd);
    return 0;
}