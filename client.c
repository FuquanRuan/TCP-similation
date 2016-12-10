/*
Name: Fuquan Ruan
Course: ICS451
Assignment #8
File: client.c
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "tcp.h"

int writeHeader(struct tcp*, struct tcp*, int);
int writeBuffer(struct tcp*, unsigned char[]);
int readBuffer(struct tcp*, unsigned char[], int);
int initalHeader(struct tcp*, int, int);
int printHeader(struct tcp*);
int printInBinary(unsigned char[]);
int sendTCPHeader(int, unsigned char[]);

/*
function: main
parameter: argc(int), argv(*[]) for user to enter server ip address and port number argument
return: 0 program ran successfully
description: create a client that connects to the server and receive data from server, save all the data into a "unknown" 
file
*/
int main(int argc, char *argv[])
{
  int sock;
  int portNumber;
  int rval;
  int status;
  struct sockaddr_in server;
  struct hostent *hp, *gethostbyname();
  unsigned char recvBuffer[6000];
  unsigned char sendBuffer[1024];
  struct tcp sendHeader;
  struct tcp recvHeader;
  FILE *wfp = fopen("unknown", "w");

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0)
  {
    perror("socket failed");
    close(sock);
    exit(1);
  }
  hp = gethostbyname(argv[1]);
  if(hp == 0)
  {
    perror("gethostbyname failed");
    close(sock);
    exit(1);
  }

  /*set up server ip address and port number*/
  portNumber = atoi(argv[2]);
  memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
  server.sin_port = htons(portNumber);
  server.sin_family = AF_INET;

  /*connect to the server*/
  if(connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
  {
    perror("connection failed");
    close(sock);
    exit(1);
  }

  /*while rval is true, server is still sending data to client*/
  initalHeader(&sendHeader, portNumber, sock);
  writeBuffer(&sendHeader, sendBuffer);
  rval = sendTCPHeader(sock, sendBuffer);

  rval = 1;
  status = 1;
  while(rval)
  {
    if((rval = recv(sock, recvBuffer, sizeof(recvBuffer), 0)) < 0)
    {
      perror("reading stream message error");
    }
    else if(rval == 0)
    {
      printf("Ending connection\n");
    }
    else
    {
      printf("\nReceived TCP Header from Server\n");
      readBuffer(&recvHeader, recvBuffer, rval);
      printHeader(&recvHeader);
      srand(time(NULL));
      /*status = 2 means connection is established. 25% to ignore the package and the recevied bytes have to bigger than 
20*/
      if(status == 2 && ((rand() % 4) == 1) && rval > 20)
      {
          printf("\n--------------------------Ignore Package--------------------------\n");
      }else
      {
        status = writeHeader(&sendHeader, &recvHeader, status);

        if(status == 1 || status == 2)
        {
          if(rval > 20)
          {
            fwrite(&(recvBuffer[20]), 1, rval - 20, wfp);
          }
          printf("\nSend TCP Header to Server\n");
          printHeader(&sendHeader);
          writeBuffer(&sendHeader, sendBuffer);
          rval = sendTCPHeader(sock, sendBuffer);

        }else if(status == 3)
        {
          printf("\nSend TCP Header to Server\n");
          printHeader(&sendHeader);
          writeBuffer(&sendHeader, sendBuffer);
          rval = sendTCPHeader(sock, sendBuffer);
          if(rval != 0)
          {
            status = writeHeader(&sendHeader, NULL, status);
            printf("\nSend TCP Header to Server\n");
            printHeader(&sendHeader);
            writeBuffer(&sendHeader, sendBuffer);
            rval = sendTCPHeader(sock, sendBuffer);
          }
        }
        if(status == 0)
        {
          rval = 0;
        }
      }



    }
  }
  fclose(wfp);
  close(sock);
  return 0;
}

/*
Function: writeHeader
Parameters:
sendHeader(struct tcp*) -> a tcp header that is going to send to the client
recvHeader(struct tcp*) -> a tcp header that received from client.
status(int) -> current status of the connection
Return:
0 -> disconnect
1 -> in handshake
2 -> connection established
3 -> recevied FIN from server
Description: Read the received headera and write a response header in sendHeader. If recvHeader is NULL, send a setted response to client
*/
int writeHeader(struct tcp *sendHeader, struct tcp *recvHeader, int status)
{
  int result;
  result = status;
  if(recvHeader != NULL)
  {
    sendHeader->sourPort = recvHeader->destPort;
    sendHeader->destPort = recvHeader->sourPort;
    sendHeader->seqNum = recvHeader->ackNum;
    if(recvHeader->syn == 1)
    {
      sendHeader->ackNum = recvHeader->seqNum + 1; //add 1 to ISN if SYN flag is on
    }else
    {
      sendHeader->ackNum = recvHeader->seqNum;
    }
    sendHeader->data = recvHeader->data;
    sendHeader->reserved = recvHeader->reserved;
    sendHeader->urg = recvHeader->urg;
    sendHeader->ack = recvHeader->ack;
    sendHeader->psh = recvHeader->psh;
    sendHeader->rst = recvHeader->rst;
    sendHeader->syn = recvHeader->syn;
    sendHeader->fin = recvHeader->fin;
    if(recvHeader->syn == 1 && recvHeader->ack == 0) //if SYN only then turn on SYN ACK flags. THIS WILL NEVER HAPPEN IN THIS CLIENT PROGRAM
    {
      sendHeader->ack = 1;
      srand(time(NULL));
      sendHeader->seqNum = rand() % 20000;
    }else if(recvHeader->syn == 1 && recvHeader->ack == 1) //if SYN ACK then turn off SYN flag, but ACK still on
    {
      sendHeader->syn = 0;
      result = 2;
      printf("\n--------------------------CONNECTION ESTABLISHED--------------------------\n");

    }else if(recvHeader->ack == 1 && status == 2)
    {
      //received data from server
      sendHeader->ackNum = recvHeader->seqNum + atoi(recvHeader->content);
    }else if(recvHeader->fin == 1 && status == 2)
    {
      //received fin, send back ack and start closing connection
      printf("\n--------------------------CLOSING CONNECTION--------------------------\n");
      sendHeader->urg = 0;
      sendHeader->ack = 1;
      sendHeader->psh = 0;
      sendHeader->rst = 0;
      sendHeader->syn = 0;
      sendHeader->fin = 0;
      result = 3;
      sendHeader->ackNum = recvHeader->seqNum + 1;

    }else if(recvHeader->fin == 1)
    {
      sendHeader->urg = 0;
      sendHeader->ack = 1;
      sendHeader->psh = 0;
      sendHeader->rst = 0;
      sendHeader->syn = 0;
      sendHeader->fin = 0;
    }else if(recvHeader->ack == 1 && status == 3)
    {
      result = 0;
    }
    sendHeader->window = recvHeader->window;
    sendHeader->checksum = recvHeader->checksum;
    sendHeader->urgentP = recvHeader->urgentP;

  }else
  {
    /*closing the connection*/
    if(status == 3)
    {
      sendHeader->urg = 0;
      sendHeader->ack = 0;
      sendHeader->psh = 0;
      sendHeader->rst = 0;
      sendHeader->syn = 0;
      sendHeader->fin = 1;
      sleep(1);
    }
  }


  return result;
}

/*
Function: writeBuffer
Parameters: header(struct tcp) for reading the receive header, buffer(char[]) for send buffer
Description: read send header and put data into send buffer
*/
int writeBuffer(struct tcp *header, unsigned char buffer[])
{
  int result;
  result = 0;

  buffer[0] = header->sourPort / 256;
  buffer[1] = header->sourPort % 256;
  buffer[2] = header->destPort / 256;
  buffer[3] = header->destPort % 256;
  buffer[4] = header->seqNum / ((int) pow(2, 24));
  buffer[5] = (header->seqNum % ((int) pow(2, 24))) / ((int) pow(2, 16));
  buffer[6] = (header->seqNum % ((int) pow(2, 16))) / ((int) pow(2, 8));
  buffer[7] = header->seqNum % ((int) pow(2, 8));
  buffer[8] = header->ackNum / ((int) pow(2, 24));
  buffer[9] = (header->ackNum % ((int) pow(2, 24))) / ((int) pow(2, 16));
  buffer[10] = (header->ackNum % ((int) pow(2, 16))) / ((int) pow(2, 8));
  buffer[11] = header->ackNum % ((int) pow(2, 8));
  buffer[12] = (header->data * ((int) pow(2, 4))) + (header->reserved % ((int) pow(2, 2)));
  buffer[13] = ((header->reserved % ((int) pow(2, 2))) * ((int) pow(2, 6))) +
  (header->urg * ((int) pow(2, 5))) +
  (header->ack * ((int) pow(2, 4))) +
  (header->psh * ((int) pow(2, 3))) +
  (header->rst * ((int) pow(2, 2))) +
  (header->syn * ((int) pow(2, 1))) +
  header->fin;
  buffer[14] = header->window / 256;
  buffer[15] = header->window % 256;
  buffer[16] = header->checksum / 256;
  buffer[17] = header->checksum % 256;
  buffer[18] = header->urgentP / 256;
  buffer[19] = header->urgentP % 256;

  //Print the sending buffer in binary format
  //printInBinary(buffer);

  return result;
}

/*
Function: readerHeader
Parameters: 
header(struct tcp) -> convert received binary data into a struct header.
tcp(unsigned char[]) -> read the received data from server.
numberofbytes(int) -> number of bytes that are recevied from server
Description: read receive tcp buffer and put data into header
*/
int readBuffer(struct tcp *header, unsigned char tcp[], int numberofbytes)
{
  int result;
  result = 0;
  header->sourPort = (tcp[0]*256) + tcp[1];
  header->destPort = (tcp[2]*256) + tcp[3];
  header->seqNum = (tcp[4] * ((int) pow(2, 24))) + (tcp[5] * ((int) pow(2, 16))) + (tcp[6]*256) + tcp[7];
  header->ackNum = (tcp[8] * ((int) pow(2, 24))) + (tcp[9] * ((int) pow(2, 16))) + (tcp[10]*256) + tcp[11];
  header->data = tcp[12]/16;
  header->reserved = ((tcp[12]%16)*4) + (tcp[13] / 64);
  header->urg = (tcp[13] / 32 ) % 2;
  header->ack = (tcp[13] / 16 ) % 2;
  header->psh = (tcp[13] / 8 ) % 2;
  header->rst = (tcp[13] / 4 ) % 2;
  header->syn = (tcp[13] / 2 ) % 2;
  header->fin = tcp[13] % 2;
  header->window = (tcp[14]*256) + tcp[15];
  header->checksum = (tcp[16]*256) + tcp[17];
  header->urgentP = (tcp[18]*256) + tcp[19];
  /*first 20 bytes of the data are header*/
  sprintf(header->content, "%d bytes", numberofbytes - 20);
  return result;
}

/*
Function: initalHeader
Parameters:
header(struct tcp*) -> the tcp header that will be initalize
portNumber(int) -> the server port number
sock(int) -> the socket number
Return:
Description: Initalize the SEND TCP Header with random sequence number, syn flag, and its own source port from the socket
*/
int initalHeader(struct tcp *header, int portNumber, int sock)
{
  int result;
  result = 0;
  srand(time(NULL));

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if(getsockname(sock, (struct sockaddr *)&sin, &len) == -1)  //get client's port number that created by the socket
  {
    perror("getsockname");
  }
  else
  {
    header->sourPort = ntohs(sin.sin_port);
    header->destPort = portNumber;
    header->seqNum = rand() % 20000;
    header->ackNum = 0;
    header->data = 5;
    header->reserved = 0;
    header->urg = 0;
    header->ack = 0;
    header->psh = 0;
    header->rst = 0;
    header->syn = 1;
    header->fin = 0;
    header->window = 0;
    header->checksum = 0;
    header->urgentP = 0;
    header->content[0] = '\0';
    printf("\n--------------------------START 3-WAY HANDSHAKE--------------------------\n");
    printf("\nSend TCP Header to Server\n");
    printHeader(header);
  }

  return result;
}

/*
Function: printHeader
Parameters: header(struct tcp*) -> a TCP header that will be print out
Return:
Description: print out the given TCP header
*/
int printHeader(struct tcp *header)
{
  int result;
  result = 0;

  printf("Source Port: %u\n", header->sourPort);
  printf("Destination Port: %u\n", header->destPort);
  printf("Sequence Number: %u\n", header->seqNum);
  printf("Acknowledgment Number: %u\n", header->ackNum);
  printf("DataOffset: %u\n", header->data);
  printf("Reserved: %u\n", header->reserved);
  printf("Flags: ");
  if(header->urg == 1)
  {
      printf("urg ");
  }
  if(header->ack == 1)
  {
      printf("ack ");
  }
  if(header->psh == 1)
  {
      printf("psh ");
  }
  if(header->rst == 1)
  {
      printf("rst ");
  }
  if(header->syn == 1)
  {
      printf("syn ");
  }
  if(header->fin == 1)
  {
      printf("fin ");
  }
  printf("\nWindow: %u\n", header->window);
  printf("Check Sum: %hx\n", header->checksum);
  printf("Urgent Pointer: %u\n", header->urgentP);
  printf("Data: %s\n", header->content);
  return result;
}

/*
Function: printInBinary
Parameters: tcp(unsigned char[]) -> a tcp binary header
Return:
Description: print out the first 20 bytes of the given tcp binary header in binary format
*/
int printInBinary(unsigned char tcp[])
{
  int result;
  result = 0;
  int i;
  int j;

  for(i = 0; i < 20; i++)
  {
    for (j = 0; j < 8; j++)
    {
       printf("%d", !!((tcp[i] << j) & 0x80));
    }
    printf("\n");
  }

  return result;
}

/*
Function: sendTCPHeader
Parameters:
mySocket(int) -> the socket number
sendBuffer(unsigned char[]) -> a TCP header that is going to send to the client
Return: 0
Description: a help function to send a TCP header to the client
*/
int sendTCPHeader(int mySocket, unsigned char sendBuffer[])
{
  int result;
  result = 1;
  //printInBinary(sendBuffer);

  if(send(mySocket, sendBuffer, 20, 0) <= 0)
  {
    perror("send failed");
    printf("client %d closed the connection\n", mySocket);
    result = 0;
  }else
  {
    printf("Send TCP Header Successfully\n");
  }

  return result;
}
