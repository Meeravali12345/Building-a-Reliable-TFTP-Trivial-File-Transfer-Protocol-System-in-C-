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
//#include "tftp_client.c"
void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("TFTP Server listening on port %d...\n", PORT);

    while (1) {
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, 
                        (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            //perror("Receive failed or timeout occurred");
            continue;
        }
        handle_client(sockfd, client_addr, client_len, &packet);
    }

    close(sockfd);
    return 0;
}

/*void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) {
    uint16_t opcode = ntohs(packet->opcode);
    
    switch (opcode) {
        case RRQ:
            printf("Read request for: %s\n", packet->body.request.filename);
            send_file(sockfd, client_addr, client_len, packet->body.request.filename);
            break;
        case WRQ:
            printf("Write request for: %s\n", packet->body.request.filename);
            tftp_packet ack_pkt;
            ack_pkt.opcode = htons(ACK);
            ack_pkt.body.ack_packet.block_number = htons(0);
            sendto(sockfd, &ack_pkt, 4, 0,
                  (struct sockaddr *)&client_addr, client_len);
            receive_file(sockfd, client_addr, client_len, packet->body.request.filename);
            break;
        default:
            printf("Unsupported operation\n");
            break;
    }
}*/
void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) {
    uint16_t opcode = ntohs(packet->opcode);
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    switch (opcode) {
        case RRQ:
            printf("Read request from %s for: %s\n", client_ip, packet->body.request.filename);
            send_file(sockfd, client_addr, client_len, packet->body.request.filename);
            break;
        case WRQ:
            printf("Write request from %s for: %s\n", client_ip, packet->body.request.filename);
            tftp_packet ack_pkt;
            ack_pkt.opcode = htons(ACK);
            ack_pkt.body.ack_packet.block_number = htons(0);
            sendto(sockfd, &ack_pkt, 4, 0,
                  (struct sockaddr *)&client_addr, client_len);
            receive_file(sockfd, client_addr, client_len, packet->body.request.filename);
            break;
        default:
            printf("Unsupported operation from %s\n", client_ip);
            break;
    }
}

