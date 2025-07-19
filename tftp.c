/*Doc
Name:
Date:
Description:
Input:
Output:
Doc
*/
#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        tftp_packet error_pkt;
        error_pkt.opcode = htons(ERROR);
        error_pkt.body.error_packet.error_code = htons(1);
        strcpy(error_pkt.body.error_packet.error_msg, "File not found");
        sendto(sockfd, &error_pkt, 4 + strlen("File not found") + 1, 0,
              (struct sockaddr *)&client_addr, client_len);
        return;
    }

    uint16_t block_num = 1;
    char buffer[512];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        tftp_packet data_pkt;
        data_pkt.opcode = htons(DATA);
        data_pkt.body.data_packet.block_number = htons(block_num);
        memcpy(data_pkt.body.data_packet.data, buffer, bytes_read);

        sendto(sockfd, &data_pkt, 4 + bytes_read, 0,
              (struct sockaddr *)&client_addr, client_len);

        tftp_packet ack_pkt;
        socklen_t len = client_len;
        int n = recvfrom(sockfd, &ack_pkt, BUFFER_SIZE, 0, 
                        (struct sockaddr *)&client_addr, &len);
        
        if (n < 0 || ntohs(ack_pkt.opcode) != ACK || 
            ntohs(ack_pkt.body.ack_packet.block_number) != block_num) {
            break;
        }

        block_num++;
    }
    fclose(file);
}*/
void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        // Error handling remains the same
        return;
    }

    uint16_t block_num = 1;
    char buffer[512];
    size_t bytes_read;
    int retries = 3;  // Add retry mechanism

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        tftp_packet data_pkt;
        data_pkt.opcode = htons(DATA);
        data_pkt.body.data_packet.block_number = htons(block_num);
        memcpy(data_pkt.body.data_packet.data, buffer, bytes_read);

        int attempt;
        for (attempt = 0; attempt < retries; attempt++) {
            sendto(sockfd, &data_pkt, 4 + bytes_read, 0,
                  (struct sockaddr *)&client_addr, client_len);

            tftp_packet ack_pkt;
            socklen_t len = client_len;
            fd_set readfds;
            struct timeval timeout = {TIMEOUT_SEC, 0};

            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
            if (ready > 0) {
                int n = recvfrom(sockfd, &ack_pkt, BUFFER_SIZE, 0,
                                (struct sockaddr *)&client_addr, &len);
                if (n > 0 && ntohs(ack_pkt.opcode) == ACK &&
                    ntohs(ack_pkt.body.ack_packet.block_number) == block_num) {
                    break;  // Success
                }
            }

            if (attempt == retries - 1) {
                printf("Error: Max retries reached for block %d\n", block_num);
                fclose(file);
                return;
            }
        }
        block_num++;
    }
    fclose(file);
}

void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        tftp_packet error_pkt;
        error_pkt.opcode = htons(ERROR);
        error_pkt.body.error_packet.error_code = htons(2);
        strcpy(error_pkt.body.error_packet.error_msg, "Cannot create file");
        sendto(sockfd, &error_pkt, 4 + strlen("Cannot create file") + 1, 0,
              (struct sockaddr *)&client_addr, client_len);
        return;
    }

    uint16_t expected_block = 1;
    tftp_packet ack_pkt;
    ack_pkt.opcode = htons(ACK);
    
    while (1) {
        tftp_packet data_pkt;
        socklen_t len = client_len;
        int n = recvfrom(sockfd, &data_pkt, BUFFER_SIZE, 0, 
                        (struct sockaddr *)&client_addr, &len);
        
        if (n < 0) break;

        if (ntohs(data_pkt.opcode) == DATA) {
            uint16_t block_num = ntohs(data_pkt.body.data_packet.block_number);
            if (block_num == expected_block) {
                size_t data_size = n - 4;
                fwrite(data_pkt.body.data_packet.data, 1, data_size, file);
                
                ack_pkt.body.ack_packet.block_number = htons(block_num);
                sendto(sockfd, &ack_pkt, 4, 0,
                      (struct sockaddr *)&client_addr, client_len);

                expected_block++;
                if (data_size < 512) break;
            }
        } else if (ntohs(data_pkt.opcode) == ERROR) {
            break;
        }
    }
    fclose(file);
}

