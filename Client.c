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

#define Chatroom_IP "127.0.0.1"
#define Chatroom_Port 8888
#define NAME_LENGTH 21
#define MSG_SIZE 151
#define LENGTH_SEND 151


volatile sig_atomic_t flag = 0;
int Socket_fd = 0;
char UserName[NAME_LENGTH] = {};


void SignalHandler(int sig) {
    flag = 1;
}

void Send_MSG()
{
    char message[MSG_SIZE] = {};
    while (1) {
        printf("\r%s", "> ");
    	fflush(stdout);
        
        while (scanf("%[^\n]%*c" , message) != NULL) {
          if (strlen(message) == 0) {
              printf("\r%s", "> ");
    	      fflush(stdout);
            } else {
                break;
            }
        }
        send(Socket_fd, message, MSG_SIZE, 0);
        if (strcmp(message, "Bye") == 0) {
            break;
        }
    }
    SignalHandler(2);
}


void Recieve_MSG() {
    char Message_recieved[LENGTH_SEND] = {};
    while (1) {
        int receive = recv(Socket_fd, Message_recieved, LENGTH_SEND, 0);
        if (receive > 0) {
            printf("\r%s\n", Message_recieved);
            printf("\r%s", "> ");
    	    fflush(stdout);
        } else if (receive == 0) {
            break;
        } else { 
            // -1 
        }
    }
}


int main()
{	
    signal(SIGINT, SignalHandler);

    // Register User Name
    printf("Please enter your name: ");
    scanf("%[^\n]%*c" , UserName);

    
    if (strlen(UserName) < 2 || strlen(UserName) >= NAME_LENGTH-1) {
        printf("\nName must be more than one and less than 20 characters.\n");
        exit(EXIT_FAILURE);
    }

    // Socket Creation - TCP
    Socket_fd = socket(AF_INET , SOCK_STREAM , 0);
    if (Socket_fd == -1) {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    // Setting Socket attributes
    struct sockaddr_in server_info, client_info;
    int s_addrlen , c_addrlen;
    
    s_addrlen = sizeof(server_info);
    c_addrlen = sizeof(client_info);
   
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = inet_addr(Chatroom_IP);
    server_info.sin_port = htons(Chatroom_Port);

    // Connecting To Server
    if(connect(Socket_fd, (struct sockaddr *)&server_info, s_addrlen) < 0)
    {
        printf("Connection to Server error!\n");
        exit(EXIT_FAILURE);
    }
    
    printf("\n--WELCOME TO PROGRAMMERS DEN--\n");
    // Names
    getsockname(Socket_fd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
    getpeername(Socket_fd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Connected to Server: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
    printf("You are: %s:%d\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

    //Sending UserName to the server
    send(Socket_fd, UserName, NAME_LENGTH, 0);

    //Sending Messages on the Chatroom
    pthread_t Send_Thread;
    if (pthread_create(&Send_Thread, NULL, (void *) Send_MSG, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }
	
    //Recieving Messages from the chatroom
    pthread_t Recieve_Thread;
    if (pthread_create(&Recieve_Thread, NULL, (void *) Recieve_MSG, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    //If pressed ctrl-c or typed exit
    while (1) {
        if(flag) {
            printf("\nHave a nice Day!\n");
            break;
        }
    }

    close(Socket_fd);
    return 0;
}