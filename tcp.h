/****************************************************************************
*
* NAME: Fuquan Ruan
*
* Homework: 8
*
* CLASS ICS451
*
* INSTRUCTOR: Ravi Narayan
*
* Date: Dec 10, 2016
*
* FILE: tcp.h
*
* DESCRIPTION: contains a struct for a header and response unsigned character array
*
******************************************************************************/

#include <stdio.h>

struct tcp
{
    unsigned short sourPort;
    unsigned short destPort;
    unsigned int seqNum;
    unsigned int ackNum;
    unsigned short data;
    unsigned short reserved;
    unsigned short urg;
    unsigned short ack;
    unsigned short psh;
    unsigned short rst;
    unsigned short syn;
    unsigned short fin;
    unsigned short window;
    unsigned short checksum;
    unsigned short urgentP;
    char content[20];
};
