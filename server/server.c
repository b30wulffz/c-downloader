#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#define PORT 8000
#define BUFFSIZE 1024
#define CHUNK_SIZE 1000000

// Check whether current file is a directory
int checkDir(char *dirName)
{
    struct stat tmp;
    stat(dirName, &tmp);

    if (S_ISDIR(tmp.st_mode))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

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

int main()
{
    int server_fd;

    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket file descriptor failed to create");
        exit(EXIT_FAILURE);
    }

    printf("> Server[%d] started successfully.\n", server_fd);

    int option = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)))
    {
        perror("Error in setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;         // ipv4
    address.sin_addr.s_addr = INADDR_ANY; // accept from any ip
    address.sin_port = htons(PORT);       // server port, htons: big endian

    // attach socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Error while binding socket to port");
        exit(EXIT_FAILURE);
    }

    // Setting upper limit for queue
    if (listen(server_fd, 3) < 0)
    {
        perror("Error while listening for client");
        exit(EXIT_FAILURE);
    }

    // wait for client(s) to connect
    while (1)
    {
        printf("> Waiting for client to connect . . .\n");
        int client_socket[2];
        for (int i = 0; i < 2; i++)
        {
            int addrlen = sizeof(address);
            if ((client_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("Error while accepting connection for client");
                exit(EXIT_FAILURE);
            }
        }
        printf("> Client %d connected (Upstream: %d, Downstream: %d).\n", client_socket[0], client_socket[1], client_socket[0]);

        int client_socket_receiver = client_socket[0];
        int client_socket_sender = client_socket[1];

        // receiving filenames
        while (1)
        {
            char *lengthFileNameStr = guarenteedReceive(client_socket_receiver, 22); // receiving length of each word
            int lengthFileName = atoi(lengthFileNameStr);

            if (lengthFileName == 0)
            {
                printf("> Client %d disconnected.\n", client_socket_receiver);
                break;
            }
            else
            {
                // check for file
                char *fileName = guarenteedReceive(client_socket_receiver, lengthFileName);
                printf("\n> '%s' is requested by client %d.\n", fileName, client_socket_receiver);

                if (checkFile(fileName))
                {
                    if (!checkDir(fileName))
                    {
                        int source_fd = open(fileName, O_RDONLY, 0);

                        if (source_fd != -1)
                        {
                            printf("> Sending '%s' to client %d.\n", fileName, client_socket_receiver);

                            long long int chunk = CHUNK_SIZE;
                            char *data_chunk = (char *)malloc(sizeof(char) * CHUNK_SIZE);

                            long long int lengthOfFile = lseek(source_fd, 0, SEEK_END);
                            lseek(source_fd, 0, SEEK_SET); // going to start position

                            sendNumber(client_socket_sender, lengthOfFile); // send length of file

                            char *status = guarenteedReceive(client_socket_receiver, 1);

                            if (strcmp(status, "1") == 0)
                            {
                                int totalLength = lengthOfFile;
                                int totalBytesSent = 0;
                                progress(0.0, 0, 0, totalLength, "Bytes");
                                while (lengthOfFile > 0)
                                {
                                    if (lengthOfFile <= chunk)
                                    {
                                        chunk = lengthOfFile;
                                    }
                                    read(source_fd, data_chunk, chunk);
                                    sendNumber(client_socket_sender, chunk); // send size of chunk
                                    guarenteedSend(client_socket_sender, data_chunk, chunk);

                                    totalBytesSent += chunk;
                                    progress((totalBytesSent * 100.0) / totalLength, 0, totalBytesSent, totalLength, "Bytes");

                                    lengthOfFile -= chunk;
                                }
                                sleep(0.1); // For better simulation

                                if (totalLength == 0)
                                {
                                    progress(100.0, 0, 0, totalLength, "Bytes");
                                }
                                printf("> '%s' transferred successfully to client %d.\n", fileName, client_socket_receiver);
                            }
                            else
                            {
                                printf("> Client %d declined '%s'.\n", client_socket_receiver, fileName);
                            }

                            free(data_chunk);
                            free(status);
                        }
                        else
                        {
                            printf("> [Error] '%s' cannot be accessed as read permissions are not granted.\n", fileName);
                            guarenteedSend(client_socket_sender, "EEEEEEEEEEEEEEEEEEE410", strlen("EEEEEEEEEEEEEEEEEEE410"));
                        }
                    }
                    else
                    {
                        printf("> [Error] '%s' is a directory.\n", fileName);
                        guarenteedSend(client_socket_sender, "EEEEEEEEEEEEEEEEEEE409", strlen("EEEEEEEEEEEEEEEEEEE409"));
                    }
                }
                else
                {
                    printf("> [Error] '%s' not found.\n", fileName);
                    guarenteedSend(client_socket_sender, "EEEEEEEEEEEEEEEEEEE404", strlen("EEEEEEEEEEEEEEEEEEE404"));
                }
            }
        }
    }
    return 0;
}