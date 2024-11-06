/*///////////////////////////////////////////////////////////
*
* FILE:  client.c
* AUTHOR: Chung To, Yixuan Yang
* PROJECT: CNT 4007 Project 2 - Professor Traynor
* DESCRIPTION: UFmyMusic
*
*////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <openssl/evp.h> 
#include <openssl/sha.h>
#define SERVER_IP "127.0.0.1"
#define PORT 8990
#define BUFFER_SIZE 100000
#define MAX_FILENAME_LEN 512
#define HASH_LEN 65
void send_command(int socket, const char *command);
void list_files_on_client(char *buffer);
void handle_list(int socket);
void handle_diff(int socket);
void handle_pull(int socket, const char * filename);
void handle_leave(int socket);
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

typedef struct Node {
    char filename[MAX_FILENAME_LEN];
    char hash[65];
    struct Node *next;
} Node;

// Function prototypes
Node* create_node(const char *filename, const char *hash) ;
void add_node(Node **head, const char *filename, const char *hash);
void display_list(Node *head);
void free_list(Node *head);

Node* local_files = NULL;
Node* server_files =NULL;
Node* diff_files = NULL;

int is_in_list(Node *head, const char *filename);
Node* find_missing_files(Node *server_list, Node *local_list);


int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    char command[BUFFER_SIZE];

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IP addresses from text to binary
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    while (1) {
        printf("Enter command (LIST, DIFF, PULL, LEAVE): ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline character

        if (strcmp(command, "LIST") == 0) {
            handle_list(client_socket);
            display_list(server_files);
        } else if (strcmp(command, "DIFF") == 0) {
            handle_diff(client_socket);
            display_list(diff_files);
        } else if (strcmp(command, "PULL") == 0) {

            handle_diff(client_socket);
            Node *current = diff_files;
            while (current != NULL) {
                handle_pull(client_socket,current->filename);
                current = current->next;
            }
            
        } else if (strcmp(command, "LEAVE") == 0) {
            handle_leave(client_socket);
            break;
        } else {
            printf("Unknown command: %s\n", command);
        }
    }

    close(client_socket);
    return 0;
}

void list_files_on_client(char *buffer) {
    DIR *d;
    struct dirent *dir;

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) { // Regular file
                strcat(buffer, dir->d_name);
                strcat(buffer, "\n");
            }
        }
        closedir(d);
    }
}

void send_command(int socket, const char *command) {
    send(socket, command, strlen(command), 0);
}

void handle_list(int socket) {
    send_command(socket, "LIST");

    char response[BUFFER_SIZE] = {0};
    int valread = read(socket, response, BUFFER_SIZE);
    if (valread > 0) {
        // printf("Server file list:\n%s\n", response);
        free_list(server_files);
        server_files = NULL;
        char *line = strtok(response, "\n");
        while (line != NULL) {
            // Process each file name (you can store them into an array or print them)
            char *colon = strchr(line, ':');
            if (colon) {
                *colon = '\0';
                add_node(&server_files, line, colon + 1);
            }
            line = strtok(NULL, "\n");
            
        }
    }
}

void handle_diff(int socket) {
    // send_command(socket, "DIFF");
    DIR *d;
    struct dirent *dir;
    handle_list(socket);
    free_list(local_files);
    free_list(diff_files);
    diff_files = NULL;
    local_files = NULL;
    char path[] ="./local_music";
    d = opendir("./local_music");
    if (d) {
            while ((dir = readdir(d)) != NULL) {
                char hash_output[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
                char filepath[1024] = {0};  // Ensure adequate buffer size
                sprintf(filepath, "%s/%s", path, dir->d_name);  // Format filepath correctly
                compute_sha256(filepath, hash_output);  // Compute hash
                // Only add regular files (not directories or special files)
                if (dir->d_type == DT_REG) {
                    add_node(&local_files, dir->d_name,hash_output);
                }
            }
    }
    diff_files = find_missing_files(server_files,local_files);
    // display_list(diff_files);

}
void get_last_three_chars(const char *input, char *output,size_t bytes) {
    int len = strlen(input);  // Get the length of the input string

    if (len >= 3) {
        for(int i=0;i<3;i++)
        {
            output[i] = input[bytes-3+i];

        }
        output[3] = '\0';
    } else {
        strcpy(output, input);  // If less than three characters, copy the whole string
    }
}
void handle_pull(int client_socket, const char *filename) {
    char buffer[BUFFER_SIZE] = {0};

    // Send a pull request to the server
    sprintf(buffer, "PULL %s", filename);
    send(client_socket, buffer, strlen(buffer), 0);
    char filepath[1024] = {};
    // buffer[bytes_r÷ead] = '\0';  // Null-terminate the incoming data for safety
    
    // Checking if the request starts with "PULL"
    sprintf(filepath,"./local_music/%s",filename);      
    // Prepare to receive the file
    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file %s for writing.\n", filename);
        return;
    }

    // Receive the file in chunksS
    size_t bytes_received;
    while (1) {
        bytes_received = read(client_socket, buffer, BUFFER_SIZE);
        char lastThree[4];  // Array to hold the last three characters, plus null terminator

        get_last_three_chars(buffer, lastThree,bytes_received);
        // if (bytes_received< BUFFER_SIZE)
        // {
        //     printf("%s\n",buffer);
        // }
        if (strcmp(lastThree, "EOF") == 0) {
            if(bytes_received>3 || bytes_received ==3)
            {
                fwrite(buffer, 1, bytes_received-3, file);
            }
            else
            {
                fwrite(buffer, 1, bytes_received, file);
            }
            
            break; // End of file transmission
        }
        
        fwrite(buffer, 1, bytes_received, file);
    }
        
    fclose(file);  // Close the file after finishing writing

    // if (bytes_received < 0) {
    //     fprintf(stderr, "Error: Failed to receive file data.\n");
    // } else {
    //     printf("File %s downloaded successfully.\n", filename);
    // }
}

void handle_leave(int socket) {
    send_command(socket, "LEAVE");
    printf("Disconnected from server.\n");
}



// Function to free the list
void free_list(Node *head) {
    Node *current = head;
    Node *next_node;
    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}

// Function to add a node to the end of the list
void add_node(Node **head, const char *filename, const char *hash)  {
    Node *new_node = create_node(filename,hash);
    if (*head == NULL) {
        *head = new_node;
    } else {
        Node *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

// Function to display the list
void display_list(Node *head) {
    Node *current = head;
    while (current != NULL) {
        printf("File: %s\n", current->filename);
        current = current->next;
    }
}

// Function to create a new node
Node* create_node(const char *filename, const char* hash) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Error allocating memory");
        exit(1);
    }
    strncpy(new_node->filename, filename, MAX_FILENAME_LEN);
    strncpy(new_node->hash, hash, HASH_LEN);
    new_node->filename[MAX_FILENAME_LEN - 1] = '\0'; // Ensure null-termination
    new_node->next = NULL;
    return new_node;
}

// Function to check if a filename exists in a list
int is_in_list(Node *head, const char *filename) {
    Node *current = head;
    while (current != NULL) {
        if (strcmp(current->hash, filename) == 0) {
            return 1; // Found
        }
        current = current->next;
    }
    return 0; // Not found
}

// Function to find files in the server list but not in the local list
Node* find_missing_files(Node *server_list, Node *local_list) {
    Node *missing_files = NULL;
    Node *current = server_list;

    while (current != NULL) {
        if (!is_in_list(local_list, current->hash)) {
            add_node(&missing_files, current->filename, current->hash);
        }
        current = current->next;
    }

    return missing_files;
}
