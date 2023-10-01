#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

/*
 * Logs the request details to a file
*/
void trace_log(char *data, struct sockaddr_in addr_from){
    FILE *logFile;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char addrBuffer[32];

    logFile = fopen("./logs/serverLogs.txt", "a");
    fprintf(logFile, "\n--------------------------------\n[%d-%02d-%02d %02d:%02d:%02d]\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    inet_ntop(AF_INET, &addr_from.sin_addr, addrBuffer, sizeof addrBuffer);
    fprintf(logFile, "Request -> %s \nIP -> %s", data, addrBuffer);
    fprintf(logFile, "\n--------------------------------\n");
    fclose(logFile);
}

/*
 * Handles the HTTP request sent by the client
*/
void handle_http_request(char *data, int descriptor, struct sockaddr_in addr_from) {
    char *req = strtok(data, " ");
    char fileData[100];
    char header200[] = "HTTP/1.1 200 OK\r\n\n";
    char header404[] = "HTTP/1.1 404 Not Found\r\n\n";
    char path[100];
    FILE *FileContent;

    printf("\nRequest type-> [%s] \n", req);
    /* Check the type of request sent by the client -> only GET is allowed */
    if(strcmp(req, "GET") == 0){
        req = strtok(NULL, " ");
        /* Move the pointer by 1 position to retrieve just the file name */
        req ++;
        printf("\nRequested file -> [%s] \n", req);

        trace_log(req, addr_from);

        /* Check if the file exists */
        sprintf(path, "./www/%s", req);
        if(access(path, F_OK) == 0){
            if (send(descriptor, header200, sizeof(header200)-1,0 ) == -1){
                printf("Error while sending the header");
                exit(1);
            }
            /* Open and send the file requested in the query */
            FileContent = fopen(path, "r");
            while(fgets(fileData, 99, FileContent) != NULL) {
                if (send(descriptor, fileData, strlen(fileData), 0) == -1) {
                    perror("Error while sending the header");
                    exit(1);
                }
            }
            /* End of data sending, close the connection */
            close(descriptor);
        }
        /* Error 404, file not found */
        else {
            if (send(descriptor, header404, sizeof(header404)-1,0 ) == -1){
                printf("Error while sending the header");
                exit(1);
            }
            FileContent = fopen("./error.html", "r");
            while(fgets(fileData, 99, FileContent) != NULL) {
                if (send(descriptor, fileData, strlen(fileData), 0) == -1) {
                    perror("Error while sending the header");
                    exit(1);
                }
            }
            close(descriptor);
        }
    }
    else {
        printf("Invalid request");
        trace_log(req, addr_from);
        exit(1);
    }
}

/*
 * Transmet le fichier de log au client
 */
void send_serverLogs(int descriptor) {
    FILE *logsFile;
    char fileData[100];
    logsFile = fopen("./logs/serverLogs.txt", "r");

    while(fgets(fileData, 99, logsFile) != NULL) {
        if (send(descriptor, fileData, strlen(fileData), 0) == -1) {
            perror("Logs file sent successfully");
            exit(1);
        }
    }
    printf("Fichier logs envoyé avec succès \n");
    close(descriptor);
}

int main(int argc, char *argv[]) {

    if (argc < 3){
        printf("Missing argument, you must specify the port number for the http server and the port number for the logs server");
        exit(1);
    }

    int skt_http;
    int skt_logs;
    struct sockaddr_in addr_http;
    struct sockaddr_in addr_logs;
    struct sockaddr_in addr_from;
    int socket_client;
    char buffer[512];
    socklen_t socket_size;
    fd_set ens1;
    u_short port_serv = atoi(argv[1]);
    u_short port_logs = atoi(argv[2]);
    int res;


    /*
     * Create server sockets
     * SOCK_STREAM -> connected mode (TCP)
     */
    skt_http = socket(AF_INET, SOCK_STREAM, 0);
    skt_logs = socket(AF_INET, SOCK_STREAM, 0);

    if (skt_http == -1 || skt_logs == -1){
        printf("Error during socket creation.");
        exit(1);
    }

    addr_http.sin_family = AF_INET;
    addr_http.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_http.sin_port = htons(port_serv);

    addr_logs.sin_family = AF_INET;
    addr_logs.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_logs.sin_port = htons(port_logs);


    /* Bind the address to the socket */
    if (bind(skt_http, (struct sockaddr*)&addr_http,sizeof addr_http) == -1){
        printf("Error during socket bind");
        exit(1);
    }

    if (bind(skt_logs, (struct sockaddr*)&addr_logs,sizeof addr_logs) == -1){
        printf("Error during socket bind");
        exit(1);
    }

    /*
     * Listening socket / max allowed connections
     */
    if (listen(skt_http, 2) == -1){
        printf("Error during socket listening");
        exit(1);
    }

    if (listen(skt_logs, 2) == -1){
        printf("Error during socket listening");
        exit(1);
    }

    while (1) {
        /* Initialization of the socket set */
        FD_ZERO(&ens1);
        /* Adding sockets to the set */
        FD_SET(skt_http, &ens1);
        FD_SET(skt_logs, &ens1);
        /* Listening to sockets using select() */
        res = select(skt_logs + 1, &ens1, NULL, NULL, NULL);
        if (res > 0) {
            /* HTTP socket */
            if (FD_ISSET(skt_http, &ens1)) {
                /*
                * Creation of the client socket upon connection
                */
                socket_client = accept(skt_http, (struct sockaddr *) &addr_from, &socket_size);
                if (socket_client == -1) {
                    printf("Error during request reception");
                    exit(1);
                }
                /*
                * Receive the HTTP request from the browser
                */
                if (recv(socket_client, buffer, sizeof buffer, 0) == -1) {
                    printf("Port http : Error during data reception");
                    exit(1);
                }
                handle_http_request(buffer, socket_client, addr_from);
            }
            /* Socket logs */
            if (FD_ISSET(skt_logs, &ens1)) {
                socket_client = accept(skt_logs, (struct sockaddr *) &addr_from, &socket_size);
                if (socket_client == -1) {
                    printf("Port logs : Error while receiving the request");
                    exit(1);
                }
                send_serverLogs(socket_client);
            }
        }
    }

    return 0;
}