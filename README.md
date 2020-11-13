# ReadMe - Creating a file downloader - Socket Programming

## Setting up environment

First navigate to the respective folders by using ```cd```.
To compile:
- For server.c: ```gcc server.c -o server```
- For client.c: ```gcc client.c -o client```

To run:
- For server.c: ```./server```
- For client.c: ```./client```

## Commands supported

Following commands are only supported in ```client.c:```
```sh
# to get the files from server
> get Filename1 Filename2 "Filename with space"
# to terminate connection from server and exit
> exit
```

## Important points to note before running the program
 - ```server.c``` and ```client.c``` should be executed in two separate terminals.
 - ```server.c``` should be executed before ```client.c```. Otherwise ```client.c``` will exit without connecting.
 - Only one client can connect to the server at a time.
 - Two sockets are created for one client. One for upstream for client and downstream for server, and another for upstream for server and downstream for client.
 - If a file is being transferred, and connection to the server gets halted, the client will stop the current operation and try again to re-establish the connection.
 - If a client gets disconnected, server will again wait for a new client to connect.
 - If a server gets disconnected, but client is waiting for user input. Upon processing get request, client program will halt.
 - If a client gets disconnected, but server is in midway of transferring the file, server program will halt. 
 - Server is running on localhost with port 8000.

## Response codes
Following responses are received from server when a client requests for a file.
| Response                | Meaning                                                               |
|-------------------------|-----------------------------------------------------------------------|
| EEEEEEEEEEEEEEEEEEE404  | Requested file does not exist on server                               |
| EEEEEEEEEEEEEEEEEEE409  | Requested file is a directory                                         |
| EEEEEEEEEEEEEEEEEEE410  | Read permissions are not granted to the server for the requested file |
| 0000000000000000123456  | Length of the chunk (eg: 123456 bytes) padded with zeros              |

Following responses are received from client when a server is about to start transfer of file.
| Response | Meaning                 |
|----------|-------------------------|
| 0        | Client rejects the file |
| 1        | Client accepts the file |

This is to deal with the case when the file with same name already exists on client end. Hence client rejects the file.

## Some assumptions
 - Buffer size is set to be 1024 Bytes.
 - Chunk size is set to be 1000000 Bytes.
 - The directory from where the shell is launched will be set as the base directory for the respective programs (i.e. server and client).
 - Files to be downloaded should be present in the base directory of the server.
 - Downloader also supports path relative to the base directory of the server.
 - Downloads will be stored in base directory of the client.
 - Directories (i.e. folders) can not be transferred.
 - Filenames containing spaces should be enclosed within " " or ' '.
 - " " or ' ' will be ignored from the filename, even though only one quote is present.
 - ```get ""``` will be treated as ```get /``` and hence transfer will be aborted.
 - If the file which is sent from the server already exists (having the same name) in the base directory of the client, then download will be cancelled.
 - Path with spaces are supported, must be enclosed within " " or ' '.
 
## Machine Specifications
 - **Operating System:** 64 bit Windows 10 Home running WSL 2 
 - **Terminal:** Microsoft Windows Subsystem for Linux Launcher
 - **Processor:** Intel Core i7-8750H CPU @ 2.20 GHz 2.21 GHz
 - **RAM:** 16 GB