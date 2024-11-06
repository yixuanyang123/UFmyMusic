/*///////////////////////////////////////////////////////////
*
* FILE:  server.c
* AUTHOR: Chung To, Yixuan Yang
* PROJECT: CNT 4007 Project 2 - Professor Traynor
* DESCRIPTION: UFmyMusic
*
*////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/evp.h> 
#include <openssl/sha.h>
#define PORT 8990
#define BUFFER_SIZE 100000
#define MAX_CLIENTS 10

void *handle_client(void *client_socket);
void list_files(int client_socket);
void diff_files(int client_socket);
void pull_files(int client_socket,char *buffer);
int compare_files(const char *file1, const char *file2);
void compute_sha256(const char *filename, char *outputBuffer) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        strcpy(outputBuffer, "File not found");
        return;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    const int bufSize = 1024;
    unsigned char buffer[bufSize];
    int bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, bufSize, file)) > 0) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }

    SHA256_Final(hash, &sha256);
    fclose(file);

    // Convert hash to hexadecimal string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
}
int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        // Accept incoming connection
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // Create a new thread for each client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_socket) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id); // Detach the thread to allow independent execution
    }

    return 0;
}

void *handle_client(void *client_socket) {
    int socket = *(int *)client_socket;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Read client message
    while ((read_size = recv(socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        char sub[5];
        if (strlen(buffer) >= 4) {  // Ensure the string has at least four characters
        strncpy(sub, buffer, 4);  // Copy the first four characters
        sub[4] = '\0';  // Manually null-terminate the new string
        } else {
            strcpy(sub, buffer);  // If less than four characters, just copy the whole string
        }
        if (strcmp(buffer, "LIST") == 0) {
            list_files(socket);
        } else if (strcmp(buffer, "DIFF") == 0) {
            diff_files(socket);
        } else if (strcmp(sub, "PULL") == 0) {
            pull_files(socket,buffer);
        } else if (strcmp(buffer, "LEAVE") == 0) {
            printf("Client disconnected.\n");
            break;
        } else {
            // continue;
            printf("Unknown command: %s\n", buffer);
        }
    }

    close(socket);
    pthread_exit(NULL);
}

void list_files(int client_socket) {
    DIR *d;
    struct dirent *dir;
    char files[BUFFER_SIZE] = "";
    char path[] = "./music"; 
    d = opendir(path);
    
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) { // Regular file
                // char filepath[1024];
                // sprintf(filepath, "%s%s", path, dir->d_name);
                // // printf('%s/n',dir->d_name);
                // strcat(files, dir->d_name);
                // strcat(files,' ');
                // char hash_output[SHA256_DIGEST_LENGTH * 2 + 1];
                // compute_sha256(filepath, hash_output);
                // strcat(files,hash_output);
                // strcat(files, "\n");
                char hash_output[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
                char filepath[1024] = {0};  // Ensure adequate buffer size
                sprintf(filepath, "%s/%s", path, dir->d_name);  // Format filepath correctly
                compute_sha256(filepath, hash_output);  // Compute hash
                strcat(files, dir->d_name);  // Append filename
                strcat(files, ":");
                strcat(files, hash_output);  // Append hash
                strcat(files, "\n");
            }
        }
        closedir(d);
    }

    send(client_socket, files, strlen(files), 0);
}

void diff_files(int client_socket) {
    char client_files[BUFFER_SIZE];
    recv(client_socket, client_files, BUFFER_SIZE, 0);
    
    DIR *d;
    struct dirent *dir;
    char server_files[BUFFER_SIZE] = "";
    char diff_result[BUFFER_SIZE] = "Files not in client:\n";

    d = opendir("./music");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                strcat(server_files, dir->d_name);
                strcat(server_files, "\n");

                // Check if the file exists in the client's file list
                if (strstr(client_files, dir->d_name) == NULL) {
                    strcat(diff_result, dir->d_name);
                    strcat(diff_result, "\n");
                }
            }
        }
        closedir(d);
    }

    // Send the diff result back to the client
    send(client_socket, diff_result, strlen(diff_result), 0);
}

void pull_files(int client_socket,char *buffer) {
    // char buffer[BUFFER_SIZE];

    // int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    int bytes_read;
    char filepath[1024] = {};
    // buffer[bytes_r÷ead] = '\0';  // Null-terminate the incoming data for safety
    
    // Checking if the request starts with "PULL"
    
    char *filename = buffer + 5;  // Extract the filename
    sprintf(filepath,"./music/%s",filename);
    
    // Open the requested file
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        char *msg = "Error: File not found\n";
        send(client_socket, msg, strlen(msg), 0);  // Send error message if file doesn't exist
        return;
    }
    // // Read the file in chunks and send it
    // (bytes_read = fread(buffer, 1, BUFFER_SIZE, file));
    // printf("%d\n",bytes_read); 
    long current_size = 0;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // printf("%d\n",bytes_read); 
        current_size+=bytes_read;
        send(client_socket, buffer, bytes_read, 0);
        // if (current_size>size || current_size == size)
        //     break;
    }
    char end_msg[] = "EOF";
    send(client_socket, end_msg, strlen(end_msg), 0);
    
    fclose(file);  // Close the file after sending
    
}

int compare_files(const char *file1, const char *file2) {
    FILE *f1 = fopen(file1, "rb");
    FILE *f2 = fopen(file2, "rb");

    if (f1 == NULL || f2 == NULL) {
        if (f1 != NULL) fclose(f1);
        if (f2 != NULL) fclose(f2);
        return 1; // Files cannot be opened or do not exist
    }

    int byte1, byte2;
    while ((byte1 = fgetc(f1)) != EOF && (byte2 = fgetc(f2)) != EOF) {
        if (byte1 != byte2) {
            fclose(f1);
            fclose(f2);
            return 1; // Files differ
        }
    }

    // Check if both files reached EOF simultaneously
    if (fgetc(f1) != EOF || fgetc(f2) != EOF) {
        fclose(f1);
        fclose(f2);
        return 1; // One file has extra content
    }

    fclose(f1);
    fclose(f2);
    return 0; // Files are the same
}
