/*Doc
Name:
Date:
Description:
Input:
Output:
Doc
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "tftp.h"
#include "tftp.c"
#include "tftp_client.h"
//#include "tftp_server.c"
 //int dis=0;

int main() {
    char command[256];
    tftp_client_t client;
    memset(&client, 0, sizeof(client));
    //dis=0;
    client.sockfd = -1;

    printf("TFTP Client (type 'help' for commands)\n");

    while (1) {
        printf("tftp> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        process_command(&client, command);
    }
    return 0;
}

void process_command(tftp_client_t *client, char *command) {
    char cmd[32];
    char arg[256];
    
    if (sscanf(command, "%s %[^\n]", cmd, arg) < 1) {
        strcpy(cmd, command);
    }
    
    if (strcmp(cmd, "connect") == 0) {
        char ip[INET_ADDRSTRLEN];
        int port = PORT;
        if (sscanf(arg, "%s %d", ip, &port) >= 1) {
            connect_to_server(client, ip, port);
        } else {
            printf("Usage: connect <server_ip> [port]\n");
        }
    } else if (strcmp(cmd, "get") == 0) {
        if (client->sockfd < 0) {
            printf("Error: Not connected\n");
            return;
        }
        get_file(client, arg);
    } else if (strcmp(cmd, "put") == 0) {
        if (client->sockfd < 0) {
            printf("Error: Not connected\n");
            return;
        }
        put_file(client, arg);
    } else if (strcmp(cmd, "bye") == 0 || strcmp(cmd, "quit") == 0) {
        disconnect(client);
        exit(0);
    } else if (strcmp(cmd, "help") == 0) {
        printf("Commands:\nconnect <ip> [port]\nget <filename>\nput <filename>\nbye/quit\nhelp\n");
    } else {
        printf("Unknown command\n");
    }
}

void connect_to_server(tftp_client_t *client, char *ip, int port) {
    if ((client->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(client->sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &client->server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client->sockfd);
        exit(EXIT_FAILURE);
    }

    client->server_len = sizeof(client->server_addr);
    strncpy(client->server_ip, ip, INET_ADDRSTRLEN);
    printf("Connected to %s:%d\n", ip, port);
}

void put_file(tftp_client_t *client, char *filename) {
    send_request(client->sockfd, client->server_addr, filename, WRQ);
    
    tftp_packet ack_pkt;
    socklen_t len = client->server_len;
    int n = recvfrom(client->sockfd, &ack_pkt, BUFFER_SIZE, 0, 
                    (struct sockaddr *)&client->server_addr, &len);
    
    if (n < 0 || ntohs(ack_pkt.opcode) != ACK || 
        ntohs(ack_pkt.body.ack_packet.block_number) != 0) {
        printf("Error: Initial ACK failed\n");
        return;
    }
    
    send_file(client->sockfd, client->server_addr, client->server_len, filename);
    printf("File %s sent\n", filename);
}

/*void get_file(tftp_client_t *client, char *filename) {
    send_request(client->sockfd, client->server_addr, filename, RRQ);
    receive_file(client->sockfd, client->server_addr, client->server_len, filename);
    printf("File %s received\n", filename);
}*/
void get_file(tftp_client_t *client, char *filename) {
    send_request(client->sockfd, client->server_addr, filename, RRQ);

    // Add proper ACK handling during file reception
    receive_file(client->sockfd, client->server_addr, client->server_len, filename);

    // Verify file was actually received
    FILE *test = fopen(filename, "r");
    if (!test) {
        printf("Error: Failed to receive file %s\n", filename);
    } else {
        fclose(test);
        printf("File %s received successfully\n", filename);
    }
}

void disconnect(tftp_client_t *client) {
    if (client->sockfd >= 0) {
        close(client->sockfd);
        client->sockfd = -1;
	//dis = 1;
        printf("Disconnected\n");
    }
}

void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode) {
    tftp_packet request;
    request.opcode = htons(opcode);
    strcpy(request.body.request.filename, filename);
    strcpy(request.body.request.mode, "octet");
    
    sendto(sockfd, &request, 4 + strlen(filename) + 1 + strlen("octet") + 1, 0,
          (struct sockaddr *)&server_addr, sizeof(server_addr));
}

