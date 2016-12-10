This program is similation of a TCP connection. including 3-way handshake, send/recv data, and closing connection process.

Unix and Linux are behaving differently on linking libraries and compile files.
Therefore, please use different make file commands in different OS.

UHUnix:
  -$make                    To create executable files for server and client in Unix OS
  -$make runserver          Run server executable file in port 25020
  -$make runclient          Run client executable file in port 25020

Linux:
  -$make linux              To create executable files for server and clinet in Linux OS
  -$make runserver          Run server executable file in port 25020
  -$make runclient          Run client executable file in port 25020

Server reads "read.png" file and send every 5000 bytes of data of the file to the client.
Client receives less or equal 5000 bytes of data from server, and save all the data into "unknown" file.
Server will send a FIN to start the closing process when all the data are sent successfully.

Client will have 25% chance to ignore the data package.
Server will re-send the package if client doesn't send back acknowledgement within 2 seconds.
