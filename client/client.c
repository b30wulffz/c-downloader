#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SERVER_PORT 8000
#define BUFFSIZE 1024

// Check whether file exists in the base directory
int checkFile(char *fileName)
{
    struct stat tmp;
    if (stat(fileName, &tmp) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// To split the input string based on delimiter
int split(char *string, char delimiter, char **dest, int buffsize)
{
    int arrInd = 0;
    if (string != NULL)
    {
        char *partition = (char *)malloc(buffsize * sizeof(char));

        int i = 0, j = strlen(string) - 1;

        int ind = 0, flag = 0, flagCodes = 0, loop = 0;

        while (i <= j)
        {
            loop = 1;
            if (string[i] == '"' || string[i] == '\'')
            {
                if (flagCodes == 0)
                {
                    flagCodes = 1;
                }
                else
                {
                    flagCodes = 0;
                }
            }
            if ((string[i] == delimiter) && flagCodes == 0)
            {
                flag = 1;
            }
            else
            {
                if (flag == 1)
                {
                    partition[ind] = '\0';
                    dest[arrInd] = partition;
                    partition = (char *)malloc(1024 * sizeof(char));
                    flag = 0;
                    ind = 0;
                    arrInd++;
                }
                if (string[i] != '"' && string[i] != '\'') // To skip single quote (') or double quote (")
                {
                    partition[ind] = string[i];
                    ind++;
                }
            }
            i++;
        }
        if (loop == 1)
        {
            partition[ind] = '\0';
            dest[arrInd] = partition;
            arrInd++;
        }
    }
    return arrInd;
}

// To extract basename from the path
char *getBaseName(char src[])
{
    char *dest = (char *)malloc(sizeof(char) * BUFFSIZE);
    int len = strlen(src);
    if (src[len - 1] == '/')
    {
        len--;
    }
    int ind = len - 1;
    while (ind >= 0)
    {
        if (src[ind] == '/')
        {
            break;
        }
        ind--;
    }
    ind++;
    int size = len - ind;
    if (size > 0)
    {
        dest = (char *)malloc(sizeof(char) * (size + 1));
        int i = 0;
        while (ind < len)
        {
            dest[i] = src[ind];
            ind++;
            i++;
        }
        dest[size] = '\0';
    }
    else
    {
        dest = (char *)malloc(sizeof(char) * 2);
        dest[0] = '/';
        dest[1] = '\0';
    }
    return dest;
}

// For progress bar
void progress(float perc, int newLine, int currentValue, int totalValue, char *unit)
{
    char prog[] = "........................................";
    int len = strlen(prog);
    int up = (int)((perc * len) / 100);
    for (int i = 0; i < up; i++)
    {
        prog[i] = '*';
    }
    char *tmpStr = (char *)malloc(sizeof(char) * 1000);
    int lenStr = 0;
    if (up != len)
    {
        if (newLine == 1)
        {
            lenStr = sprintf(tmpStr, "\r[ %s ] %0.2f%% - [%d %s/%d %s]\n", prog, perc, currentValue, unit, totalValue, unit);
        }
        else
        {
            lenStr = sprintf(tmpStr, "\r[ %s ] %0.2f%% - [%d %s/%d %s]", prog, perc, currentValue, unit, totalValue, unit);
        }
    }
    else
    {
        lenStr = sprintf(tmpStr, "\r[ %s ] %0.2f%% - [%d %s/%d %s]\n", prog, perc, currentValue, unit, totalValue, unit);
    }
    write(1, tmpStr, lenStr);
    free(tmpStr);
}

// To send to the socket
int guarenteedSend(int socket, char *data, int len)
{
    int sentLen = 0;
    int leftBytes = len;
    while (sentLen < len)
    {
        int sentBytes = send(socket, data + sentLen, leftBytes, 0);
        if (sentBytes != -1)
        {
            sentLen += sentBytes;
            leftBytes -= sentLen;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// To receive from the socket
char *guarenteedReceive(int client_socket_receiver, int len)
{
    char *buffer = (char *)malloc(sizeof(char) * (len + 1));

    int leftBytes = len;
    int count = 0;
    int total = 0;
    do
    {
        count = recv(client_socket_receiver, buffer + total, leftBytes, 0);
        total += count;
        leftBytes -= count;
    } while (count > 0 && leftBytes > 0);
    *(buffer + total) = '\0';
    return buffer;
}

// To send a number to the socket
int sendNumber(int client_socket_sender, long long int number)
{
    char *numberChar = (char *)malloc(sizeof(char) * 23);
    int len = sprintf(numberChar, "%.22lld", number);
    return guarenteedSend(client_socket_sender, numberChar, len);
}

int getFiles(int client_socket_sender, int client_socket_receiver, char **parsed, int parsedLength)
{
    int disconnected = 0;
    if (parsedLength > 1)
    {
        for (int i = 1; i < parsedLength; i++)
        {
            char *filePath = parsed[i];
            char *fileName = getBaseName(parsed[i]);

            if (strcmp(fileName, "/") == 0) // handling case when filename == "/"
            {
                filePath = fileName;
            }

            printf("\n> For %s:\n", fileName);

            sendNumber(client_socket_sender, strlen(filePath)); // sending length of file path

            guarenteedSend(client_socket_sender, filePath, strlen(filePath));

            char *response = guarenteedReceive(client_socket_receiver, 22);

            if (strlen(response) > 0)
            {
                if (response[0] == 'E')
                {
                    if (strcmp(response, "EEEEEEEEEEEEEEEEEE404") == 0)
                    {
                        printf("> [Error] '%s' not found.\n", fileName);
                    }
                    else if (strcmp(response, "EEEEEEEEEEEEEEEEEE409") == 0)
                    {
                        printf("> [Error] '%s' is a directory.\n", fileName);
                    }
                    else if (strcmp(response, "EEEEEEEEEEEEEEEEEE410") == 0)
                    {
                        printf("> [Error] '%s' cannot be accessed as read permissions are not granted to the server\n", fileName);
                    }
                    else
                    {
                        printf("> [Error].\n");
                    }
                }
                else
                {
                    int datalen = atoi(response); // extracting total length(size) of the file
                    printf("> Processing '%s' of %d bytes...\n", fileName, datalen);
                    if (!checkFile(fileName)) // check whether file exists in the base directory
                    {
                        guarenteedSend(client_socket_sender, "1", strlen("1"));
                        int dest_fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0777); // create a destination file with append mode
                        int receivedLen = 0;
                        progress(0.0, 0, 0, datalen, "Bytes");
                        while (receivedLen < datalen)
                        {
                            char *chunkLength = guarenteedReceive(client_socket_receiver, 22); // receiving size of chunk
                            int chunk = atoi(chunkLength);
                            if (chunk != 0)
                            {
                                char *data = guarenteedReceive(client_socket_receiver, chunk); // receiving data chunk
                                write(dest_fd, data, chunk);                                   // writing data chunk to the destination file
                                receivedLen += chunk;
                                progress((receivedLen * 100.0) / datalen, 0, receivedLen, datalen, "Bytes");
                                free(data);
                            }
                            else
                            {
                                progress((receivedLen * 100.0) / datalen, 1, receivedLen, datalen, "Bytes");
                                printf("> [Error] Disconnected from server. Halting operation...\n");
                                disconnected = -1;
                            }
                            free(chunkLength);
                            if (disconnected == -1)
                            {
                                break;
                            }
                        }
                        close(dest_fd);
                        if (disconnected != -1)
                        {
                            if (datalen == 0)
                            {
                                progress(100.0, 0, 0, datalen, "Bytes");
                            }
                            printf("> '%s' downloaded successfully.\n", fileName);
                        }
                    }
                    else
                    {
                        printf("> [Error] A file with same name '%s' already exist in the downloads.\n", fileName);
                        guarenteedSend(client_socket_sender, "0", strlen("0"));
                    }
                }
            }
            else
            {
                printf("> [Error] Disconnected from server. Halting operation...\n");
                disconnected = -1;
            }

            free(response);
            if (disconnected == -1)
            {
                break;
            }
        }
    }
    else
    {
        printf("> [Error] Unsufficient arguments supplied\n");
    }
    if (disconnected == -1)
    {
        return -1;
    }
    return 0;
}

int main()
{
    int client_socket_fd_1, client_socket_fd_2;

    // creating socket file descriptor
    if ((client_socket_fd_1 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket file descriptor failed to create");
        exit(EXIT_FAILURE);
    }
    // creating socket file descriptor
    if ((client_socket_fd_2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket file descriptor failed to create");
        exit(EXIT_FAILURE);
    }

    // Initializing server address
    struct sockaddr_in server_addr;

    memset(&server_addr, '0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        printf("\nError, Invalid address: Address not supported \n");
        exit(EXIT_FAILURE);
    }

    printf("> Trying to establish connection with the server.\n");
    // Establishing connection to server for upstream
    if (connect(client_socket_fd_1, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("\nConnection to server failed \n");
        exit(EXIT_FAILURE);
    }

    // Establishing connection to server for downstream
    if (connect(client_socket_fd_2, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("\nConnection to server failed \n");
        exit(EXIT_FAILURE);
    }

    int client_socket_sender = client_socket_fd_1;
    int client_socket_receiver = client_socket_fd_2;

    printf("> Connection with the server established successfully.\n");

    char **parsed = (char **)malloc((sizeof(char) * BUFFSIZE) * BUFFSIZE);
    int disconnect = 0;

    while (1)
    {
        char *input = malloc(sizeof(char) * BUFFSIZE);
        printf("client> ");
        gets(input);

        int parsedLength = split(input, ' ', parsed, BUFFSIZE); // splitting input
        if (parsedLength > 0)
        {
            if (strcmp(parsed[0], "exit") == 0)
            {
                break;
            }
            else if (strcmp(parsed[0], "get") == 0)
            {
                disconnect = getFiles(client_socket_sender, client_socket_receiver, parsed, parsedLength);
            }
            printf("\n");
        }

        free(input);
        if (disconnect == -1)
        {
            close(client_socket_fd_1);
            close(client_socket_fd_2);
            printf("Attempting to reconnect ...\n");
            if ((client_socket_fd_1 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Socket file descriptor failed to create");
                exit(EXIT_FAILURE);
            }
            if ((client_socket_fd_2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Socket file descriptor failed to create");
                exit(EXIT_FAILURE);
            }
            // reconnecting upstream socket
            while (connect(client_socket_fd_1, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
                ;
            printf("> Upstream connected\n");
            // reconnecting downstream socket
            while (connect(client_socket_fd_2, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
                ;
            printf("> Downstream connected\n");
            printf("> Connection restablished successfully.\n");
        }
    }
    close(client_socket_fd_1);
    close(client_socket_fd_2);
    free(parsed);
    return 0;
}