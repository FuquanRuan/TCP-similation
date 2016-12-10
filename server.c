/*
Name: Fuquan Ruan
Course: ICS451
Assignment #8
File: server.c
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <fcntl.h>
#include "tcp.h"


void *func(void *sockID);
int writeHeader(struct tcp*, struct tcp*, int);
int writeBuffer(struct tcp*, unsigned char[]);
int readBuffer(struct tcp*, unsigned char[]);
int printHeader(struct tcp*);
int printInBinary(unsigned char[]);
int sendTCPHeader(int, unsigned char[], int);
/*
function: main
parameter: argc(int), argv(*[]) for user to enter port number argument
return: 2 bind failed, 1 cannot create new thread error, 0 program ran successfully
description: create a server using socket and use mutithreading to handle mutiple clients
*/
int main(int argc, char *argv[])
{
  struct sockaddr_in server;
  int sock;
  int portNumber;
  portNumber = atoi(argv[1]);
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0)
  {
    perror("Failed to create socket\n");
    exit(1);
  }
  printf("Create socket sucessfully\n");
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(portNumber);

  if(bind(sock, (struct sockaddr *) &server, sizeof(server)))
  {
    perror("bind failed");
    return 2;
  }
  printf("Bind sucessfully\n");
  printf("Waiting for clients\n------------------------\n");

  listen(sock, 5);
  int mySocket;
  int *newsock;
  signal(SIGPIPE, SIG_IGN);
  while((mySocket = accept(sock, (struct sockaddr *) 0, 0)) > 0)
  {
    pthread_t newThread;
    newsock = malloc(1);
    *newsock = mySocket;
    if(pthread_create(&newThread, NULL, func, (void *) newsock) < 0)
    {
      perror("Cannot create new thread!\n");
      return 1;
    }
  }

  return 0;
}

/*
function: *func
parameter: sockID(void*) socket number passed by pthread_create function from main
return: void
description: a thread function for send/receive TCP header to/from client
*/
void *func(void *sockID)
{
    int mySocket = *(int*) sockID;
    unsigned char sendBuffer[5021];
    unsigned char recvBuffer[1024];
    struct tcp sendHeader;
    struct tcp recvHeader;
    FILE *rfp = fopen("read.png", "r");
    int rcount = 0;
    if(mySocket == -1)
    {
      perror("accept failed");
    }
    else
    {
      /*clean the buff*/

      printf("\n--------------------------START 3-WAY HANDSHAKE--------------------------\n");
      /* using socket options for receiving timeout. However, it is not working in UHUnix
      struct timeval timeout;
      timeout.tv_sec = 5;
      timeout.tv_usec = 0;
      setsockopt (mySocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
      */

      int flag;
      int rval = 0;
      int status = 0;
      int passedTime = 0;
      int timeout = 2;

      //change control flag to unblock so recv function will not wait message to come
      if ((flag = fcntl(mySocket, F_GETFL, 0)) < 0){
        perror("ERROR: Get control flags failed.\n");
      }else
      {
        if(fcntl(mySocket, F_SETFL, flag | O_NONBLOCK)<0)
        {
          perror("ERROR: Set control flags failed.\n");
        }else
        {
          rval = 1;
          status = 1;
        }
      }


      while(rval)
      {
        //memset(sendBuffer, 0, sizeof(sendBuffer));
        memset(recvBuffer, 0, sizeof(recvBuffer));
        if((rval = recv(mySocket, recvBuffer, 20, 0)) < 0)
        {
          if(passedTime >= timeout)
          {
            //Client didn't response, resend the previous package
            printf("\n--------------------------TIMEOUT Re-Sending--------------------------\n");
            //writeBuffer(&sendHeader, sendBuffer);
            rval = sendTCPHeader(mySocket, sendBuffer, rcount);
            passedTime = 0;
          }else
          {
            passedTime++;
            sleep(1); //sleep one second for timeout
            printf("\nWaiting for client's message for %d second(s)\n", passedTime);
          }
        }else if(rval == 0)
        {
          printf("Ending connection\n");
        }else if(rval < 20)
        {
          printf("**************************Received package is less than 20 bytes, ignore package***************************\n");
        }
        else
        {
          passedTime = 0;
          /*receive buffer from client*/
          readBuffer(&recvHeader, recvBuffer);
          /*write back tcp response to client*/
          status = writeHeader(&sendHeader, &recvHeader, status);
          if(status <= 3)
          {
            rcount = 0;

            if(status == 2)
            {
              if(!feof(rfp))
              {
                rcount = fread(&(sendBuffer[20]), 1, 5000, rfp);
                printf("number of bytes that is going to be sent %d\n", rcount);
                if(rcount < 5000)
                {
                  status = 8;
                }
              }
            }
            sprintf(sendHeader.content, "%d bytes", rcount);
            writeBuffer(&sendHeader, sendBuffer);
            /*send data now*/
            rval = sendTCPHeader(mySocket, sendBuffer, rcount);

          }

          if(status == 0) //close the connection
          {
            rval = 0;
          }
        }
      }

      printf("Disconnecting to client %d\n", mySocket);
      close(mySocket);
    }
    fclose(rfp);
    free(sockID);
    return 0;
}

/*
Function: writeBuffer
Parameters:
sendHeader(struct tcp*) -> a tcp header that is going to send to the client
recvHeader(struct tcp*) -> a tcp header that received from client.
status(int) -> the current status of the connection
Return:
0 -> disconnect from server
1 -> in handshake
2 -> connection established
3 -> send ACK for received data
4 -> got FIN, send ACK back to client
Description: Read the received headera and write a response header in sendHeader. If recvHeader is NULL, send a setted response to client
*/
int writeHeader(struct tcp *sendHeader, struct tcp *recvHeader, int status)
{
  int result;
  result = status;
  //time_t currentTime;
  if(recvHeader != NULL)
  {
    sendHeader->sourPort = recvHeader->destPort;
    sendHeader->destPort = recvHeader->sourPort;
    sendHeader->seqNum = recvHeader->ackNum;
    sendHeader->content[0] = '\0';
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
    if(recvHeader->syn == 1 && recvHeader->ack == 1) //if SYN ACK then turn off SYN flag, but ACK still on
    {
      sendHeader->syn = 0;
    }else if(recvHeader->syn == 1) //if SYN only then turn on SYN ACK flags. THIS WILL NEVER HAPPEN IN THIS CLIENT PROGRAM
    {
      sendHeader->ack = 1;
      srand(time(NULL));
      sendHeader->seqNum = rand() % 2000;

    }else if((recvHeader->fin == 1 && status == 4) || (recvHeader->fin == 1 && status == 3 && recvHeader->ack == 1)) //if FIN then only turn on ACK flag. THIS WILL NEVER HAPPEN IN THIS CLIENT PROGRAM
    {
      sendHeader->urg = 0;
      sendHeader->ack = 1;
      sendHeader->psh = 0;
      sendHeader->rst = 0;
      sendHeader->syn = 0;
      sendHeader->fin = 0;
      result = 0;
      sendHeader->ackNum = recvHeader->seqNum + 1;
    }else if(recvHeader->syn == 0 && recvHeader->ack == 1 && status == 1)
    {
      //time(&currentTime);
      //sprintf(sendHeader->content, "%s", ctime(&currentTime));
      result = 2;
      printf("\n--------------------------CONNECTION ESTABLISHED--------------------------\n");

    }else if(recvHeader->ack == 1 && status == 8) //status 8 means start closing
    {
      printf("\n--------------------------CLOSING CONNECTION--------------------------\n");
      sendHeader->urg = 0;
      sendHeader->ack = 0;
      sendHeader->psh = 0;
      sendHeader->rst = 0;
      sendHeader->syn = 0;
      sendHeader->fin = 1;
      result = 3;
    }else if(recvHeader->ack == 1 && status == 3)
    {
      result = 4;
    }
    sendHeader->window = recvHeader->window;
    sendHeader->checksum = recvHeader->checksum;
    sendHeader->urgentP = recvHeader->urgentP;
  }else
  {
    if(status == 3)
    {
      printf("\n--------------------------CLOSING CONNECTION--------------------------\n");
      sendHeader->urg = 0;
      sendHeader->ack = 0;
      sendHeader->psh = 0;
      sendHeader->rst = 0;
      sendHeader->syn = 0;
      sendHeader->fin = 1;
      sleep(1);
    }
  }
  /*
  printf("status = %d\n", status);
  */
  return result;
}

/*
Function: writeBuffer
Parameters: header(struct tcp) for reading the receive header, buffer(char[]) for send buffer
Return: 0
Description: convert send header into send buffer
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
  //need to change to another write content function, strcpy() can be interaped by '\0'
  //strcpy(&(buffer[20]), header->content);
  printf("\nSend TCP Header to Client\n");
  printHeader(header);
  return result;
}


/*
Function: readerHeader
Parameters: 
header(struct tcp) -> header that will contain the received data
tcp(unsigned char[]) -> buffer that contains the received data
Return: 0
Description: convert received data in the buffer to a struct header
*/
int readBuffer(struct tcp *header, unsigned char tcp[])
{
  //printInBinary(tcp);
  int result;
  result = 0;
  header->sourPort = (tcp[0] << 8) + tcp[1];
  header->destPort = (tcp[2] << 8) + tcp[3];
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
  printf("\nReceive TCP Header from Client\n");
  printHeader(header);
  return result;
}

/*
Function: printHeader
Parameters: header(struct tcp*) is a tcp header struct
Return: 0
Description: print out a struct tcp header
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
Parameters: tcp(unsigned char[]) is a tcp header
Return: 0
Description: print a 20 bytes tcp header in binary formate
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
int sendTCPHeader(int mySocket, unsigned char sendBuffer[], int numberofbytes)
{
  int result;
  result = 1;

  if(send(mySocket, sendBuffer, 20 + numberofbytes, 0) <= 0)
  {
    perror("send failed");
    printf("client %d closed the connection\n", mySocket);
    result = 0;
  }
  else
  {
    printf("Send TCP Header Successfully\n");
  }

  return result;
}
