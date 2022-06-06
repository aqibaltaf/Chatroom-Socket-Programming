#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server.h"

#define NAME_LENGTH 21
#define MSG_SIZE 151
#define LENGTH_SEND 151


int socket_server = 0, socket_client = 0;
ClientList *Head, *current_node;

void SignalHandler(int sig) {
    ClientList *temp;
    while (Head != NULL) {
        printf("\nClose socketfd: %d\n", Head->data);
        close(Head->data); // close all socket include socket_server
        temp = Head;
        Head = Head->next;
        free(temp);
    }
    printf("Bye\n");
    exit(0);
}

void Broadcast(ClientList *np, char temp_buffer[]) {
    ClientList *temp = Head->next;
    while (temp != NULL) {
        if (np->data != temp->data) { // all clients except itself.
            printf("Send to sockfd %d: \"%s\" \n", temp->data, temp_buffer);
            send(temp->data, temp_buffer, LENGTH_SEND, 0);
        }
        temp = temp->next;
    }
}

void SendRecieve_handler(void *p_client) {
    int leave_flag = 0;
    char UserName[NAME_LENGTH] = {};
    char Recieve_Buffer[MSG_SIZE] = {};
    char Send_Buffer[LENGTH_SEND] = {};
    ClientList *np = (ClientList *)p_client;

    // Naming
    if (recv(np->data, UserName, NAME_LENGTH, 0) <= 0 || strlen(UserName) < 2 || strlen(UserName) >= NAME_LENGTH-1) {
        printf("%s didn't input name.\n", np->ip);
        leave_flag = 1;
    } else {
        strncpy(np->name, UserName, NAME_LENGTH);
        printf("%s(%s)(%d) join the chatroom.\n", np->name, np->ip, np->data);
        sprintf(Send_Buffer, "%s(%s) join the chatroom.", np->name, np->ip);
        Broadcast(np, Send_Buffer);
    }

    // Conversation
    while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(np->data, Recieve_Buffer, MSG_SIZE, 0);
        if (receive > 0) {
            if (strlen(Recieve_Buffer) == 0) {
                continue;
            }
            sprintf(Send_Buffer, "%sï¼š%s", np->name, Recieve_Buffer);
        } else if (receive == 0 || strcmp(Recieve_Buffer, "exit") == 0) {
            printf("%s(%s)(%d) leave the chatroom.\n", np->name, np->ip, np->data);
            sprintf(Send_Buffer, "%s(%s) leave the chatroom.", np->name, np->ip);
            leave_flag = 1;
        } else {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
        Broadcast(np, Send_Buffer);
    }

    // Remove Node
    close(np->data);
    if (np == current_node) { // remove an edge node
        current_node = np->prev;
        current_node->next = NULL;
    } else { // remove a middle node
        np->prev->next = np->next;
        np->next->prev = np->prev;
    }
    free(np);
}

int main()
{
    signal(SIGINT, SignalHandler);

    // Socket Creation - TCP
    socket_server = socket(AF_INET , SOCK_STREAM , 0);
  
    
    if (socket_server == -1) {
       printf("Fail to create a socket.");
      exit(EXIT_FAILURE);
    }

    // Socket attributes
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(8888);

    // Bind and Listen
    bind(socket_server, (struct sockaddr *)&server_info, s_addrlen);
    listen(socket_server, 2);

    // Print Server IP
    getsockname(socket_server, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Server Started on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    // Creating a nexted list for clients
    Head = newNode(socket_server, inet_ntoa(server_info.sin_addr));
    current_node = Head;

    while (1) {
        socket_client = accept(socket_server, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);

        // Get Client IP
        getpeername(socket_client, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
        printf("Client %s:%d come in.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

        // Append nexted list for clients
        ClientList *clientNode = newNode(socket_client, inet_ntoa(client_info.sin_addr));
        clientNode->prev = current_node;
        current_node->next = clientNode;
        current_node = clientNode;

        pthread_t id;
        if (pthread_create(&id, NULL, (void *)SendRecieve_handler, (void *)clientNode) != 0) {
            perror("Create pthread error!\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}