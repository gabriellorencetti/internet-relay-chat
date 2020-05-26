 /*
    Computer Network SSC-0142

    ---- Internet Relay Chat ----
    Module 1 - Sockets Implementation

    Caio Augusto Duarte Basso NUSP 10801173
    Gabriel Garcia Lorencetti NUSP 10691891

    Client

 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <thread>
#include <atomic>
#include <signal.h> 

#define BUFFER_SIZE 2048 // max char amount of buffer
#define REC_BUFFER_SIZE 4096 // 2x max char amount of buffer (used if the message exceeds BUFFER_SIZE)
#define NICK_SIZE 50 // max char amount of nickname
#define BUFFER_SIZE_MAX 40960 // max char amount of bufferMax

void userCommand(char*);
void readNickname();
void sendController();
void receiveController();
void connectServer(char*, int);
void print_commands();


struct sockaddr_in serverAddress;
struct hostent *server;    
char nickname[NICK_SIZE]; // nickname of the client
char buffer[BUFFER_SIZE]; // message to write to client
char recBuffer[REC_BUFFER_SIZE]; // message to write to client (used if the message exceeds BUFFER_SIZE)
char bufferMax[BUFFER_SIZE_MAX]; // max message (used if the message exceeds BUFFER_SIZE)
char message[BUFFER_SIZE+NICK_SIZE+2]; // message to write to client + nickname
int socketClient, portNum, n; // informations of the client
std::atomic<bool> flag (false); // flag used to stop the application
std::atomic<bool> connected (false); // flag used to stop the application

/*
    Function that prints the error and returns 1.

*/
void error(const char *msg){
    perror(msg);
    exit(1);
}

/*
    Function that sends a message to the server.

*/
void sendMessage(char bufferMax[]){

    // In case of the message exceeds the BUFFER_SIZE, splits in several messages;
    if(strlen(bufferMax) > BUFFER_SIZE){
        int tam = strlen(bufferMax);
        int aux = 0;

        while(tam > BUFFER_SIZE){
            strncpy(buffer, bufferMax+aux, BUFFER_SIZE);
            buffer[BUFFER_SIZE] = '\0';

            if (aux == 0)
                sprintf(message, "%s: %s", nickname, buffer);
            else sprintf(message, "\n%s: %s", nickname, buffer);
            
            n = write(socketClient, message, strlen(message));    // sends message to server
            if (n == -1) 
                error("!!! Error writing to socket ");

            bzero(buffer, BUFFER_SIZE); // clears buffer
            bzero(message,BUFFER_SIZE+NICK_SIZE+2); // clears message

            tam -= BUFFER_SIZE;
            aux += BUFFER_SIZE;
        }
        strncpy(buffer, bufferMax+aux, tam);
    }
    else{
        strcpy(buffer, bufferMax);
    }

    sprintf(message, "%s: %s", nickname, buffer);
    n = write(socketClient, message, strlen(message));    // sends message to server
    if (n == -1) 
        error("!!! Error writing to socket ");
}

/*
    Function that verifies the commands typed by the user.

*/
void userCommand(char message[]){
    if (strcasecmp(message, "/quit") == 0){    // if user wants to leave chat
        flag = true;
        connected = true;
    }

    else if (strcasecmp(message, "/connect") == 0){ // if user wants to connect to server
        char tempConnect[256];
        printf("\n-> Type the port number to connect to the server. The default port used is 52547.\n->If you want to cancel connection, type ABORT\n(Keep in mind that if you already connected to another chat, it will disconnect from current room!)\n");
        
        fgets(tempConnect, 256, stdin);
        tempConnect[strlen(tempConnect)-1] = '\0';

        if(strcasecmp(tempConnect, "ABORT") == 0)
            return;
        
        char localhost[] = "localhost";
        connectServer(localhost, atoi(tempConnect));

    }
    else if (strcasecmp(message, "/ping") == 0){    // if user wants to check latency to server
        if(connected) sendMessage(message);
        else printf("\n->You are not connected to any chat yet!\n-> Use the /connect command first to connect to a server.\n\n");
    }
    else if (strcasecmp(message, "/help") == 0){    // if user asks for commands
        print_commands();
        printf("\n");
    }
}

/*
    Function that reads the client's nickname.

*/
void readNickname(){

    char buffer_nick[256];

    printf("-> Type your nickname (maximum of fifty characters): ");
    fgets(buffer_nick, 256, stdin);

    if(strlen(buffer_nick) > 50){
        printf("-> The nickname exceeds 50 characters. The first 50 characters will be considered.\n");
        buffer_nick[NICK_SIZE] = '\0';
    }
    else{
        buffer_nick[strlen(buffer_nick)-1] = '\0';
    }
    
    strcpy(nickname, buffer_nick);
    buffer_nick[0] = '\0';
}

/*
    Function responsible for sending messages to the server.
*/
void sendController(){

    while(!flag){
        bzero(bufferMax,BUFFER_SIZE_MAX); // clears bufferMax
        bzero(message,BUFFER_SIZE+NICK_SIZE+2); // clears message
        fgets(bufferMax, BUFFER_SIZE_MAX, stdin); // reads message from input
        bufferMax[strlen(bufferMax)-1] = '\0';
        
        if (feof(stdin)) strcpy(bufferMax, "/quit");
        if(bufferMax[0] == '/'){ // if it's a command, check 
            userCommand(bufferMax);
        }

        else {
            sendMessage(bufferMax);
        }
    }
}

/*
    Function responsible for receiving messages from the server.
*/
void receiveController(){
    while(!flag){
        bzero(recBuffer,REC_BUFFER_SIZE); // clears recBuffer
        n = read(socketClient, recBuffer, REC_BUFFER_SIZE); // receives message from server
        if (n == -1) 
            error("!!! Error reading from socket !!!");
        else if (n == 0 || strcasecmp("quit\n", recBuffer) == 0) { // if the server is down
            printf("Server has left");
            flag = true;
        }
        else if(n > 1) printf("%s\n",recBuffer); // prints message
    }
}

void connectServer(char *serverName, int serverPort){

    portNum = serverPort; // sets port number;
    socketClient = socket(AF_INET, SOCK_STREAM, 0); // creates a socket;
    if (socketClient == -1)
        error("!!! Error while opening socket !!!\n");

    server = gethostbyname(serverName); // return entry from host data base for host with NAME.

    if (server == NULL)
        error("!!! Error, host not found !!!\n");
    
    bzero( (char *) &serverAddress, sizeof(serverAddress)); // initializes serverAddress with 0s    

    serverAddress.sin_family = AF_INET; // setting serverAddress values
    bcopy( (char *)server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length);   // copies server IP address into serverAddress
    serverAddress.sin_port = htons(portNum);    // htons() method converts int into network byte order

    if (connect(socketClient, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
        error("!!! Error connecting !!!\n");

    n = read(socketClient, buffer, 4095); // receives message from server
    if (n == -1)    // message should confirm connection success
         error("!!! Error reading from socket !!!");

    // If the chat is already full, exits
    if(strcasecmp(buffer, "Chat full") == 0){
        printf("---- Sorry, but the chat is full. Connection refused. ;( ----\n");
        close(socketClient);
        exit(1);
    }
    connected = true;

    printf("\n%s", buffer);

    // reads nickname and sends it to the server;
    readNickname();
    write(socketClient, nickname, NICK_SIZE);    

    printf("\n-> Hi, %s. The chat is ready for conversation!\n-> Type \"/quit\" at any time to leave the chat and \"/help\" to see the other commands.\n\n", nickname);

    // if the code gets here without any error, it is connected and ready to send and receive messages; 

    //Creating two threads, one to send messages and one to receive messages;
    std::thread sendMessages (sendController);
    sendMessages.detach();
    std::thread receiveMessages (receiveController);
    receiveMessages.detach();
}

void first_connection(){
    bzero(bufferMax,BUFFER_SIZE); // clears message
    fgets(bufferMax, BUFFER_SIZE_MAX, stdin); // reads message from input
    bufferMax[strlen(bufferMax)-1] = '\0';
    
    
    if (feof(stdin)) strcpy(bufferMax, "/quit");
    if(bufferMax[0] == '/'){ // if it's a command, check 
        userCommand(bufferMax);
    }
    
}

void print_commands(){
    printf("\n-> Commands:\n\n");
    printf("   - /connect (establishes the connection to the server given a port)\n");
    printf("   - /quit (ends the connection and closes the application)\n");
    printf("   - /ping (the server returns pong as soon as it receives the message)\n");
}

void print_welcome_message(){
    printf("\n --------------------------------------");
    printf("\n|                                      |");
    printf("\n|         Welcome to the chat!         |");
    printf("\n|                                      |");
    printf("\n --------------------------------------\n");

    print_commands();
    printf("   - /help (shows available commands)\n\n");

}

void sigintHandler(int sig_num){
    signal(SIGINT, sigintHandler); 
    printf("\n---- Cannot be terminated using Ctrl+C ----\n"); 
    printf("-> Instead, use the /quit command or Ctrl+D.\n");
    fflush(stdout); 
} 

int main(int argc, char *argv[]){
    
    signal(SIGINT, sigintHandler);

    print_welcome_message();

    while(!connected) first_connection();

    // while not receiving the signal to disconnect, continues...
    while(!flag){}

    printf("\n--- Leaving the application... Goodbye! ---\n\n");

    // Close the socket and return 0;
    close(socketClient);
    return 0;
}