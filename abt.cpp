#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#include <iostream>
#include <stdio.h>
#include <queue>
#include <string.h>

using namespace std;
#define A 0
#define B 1
#define TIMEOUT 20.0
#define WAITLAYER3 0
#define WAITLAYER5 1

//message buffer
queue<msg> msg_buffer;

//

//create variables for sender
struct Sender
{
    //the sequnce number for the packet
    int seqnum;
    //the expected ack number for the pakect 
    int acknum;
    //state: wait up player (layer 5) or wait down layer (layer 3)
    int state;
}host_a;

//create variables for receiver
struct Receiver
{
    int acknum;
}host_b;


//calculate the checksum of a packet
int get_checksum(pkt packet)
{
    //cout << "checksum start" << endl;
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < 20; ++i)
    {
        checksum += packet.payload[i];
    }
    //cout << "checksum end" << endl;
    return checksum;
}

//make packet for message
pkt make_packet(int seqnum, msg message) 
{
    pkt packet;
    packet.seqnum = seqnum;
    packet.acknum = seqnum;
    strncpy(packet.payload, message.data, 20);
    packet.checksum = get_checksum(packet);
    return packet;
}

pkt make_ACKpacket(int acknum)
{
    pkt packet;
    packet.seqnum = 0;
    packet.acknum = acknum;
    memset(packet.payload, 0, sizeof(packet.payload));
    packet.checksum = get_checksum(packet);
    return packet;

}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    msg_buffer.push(message);
    if (host_a.state == WAITLAYER5)
    {
        pkt packet = make_packet(host_a.seqnum, message);
        tolayer3(A, packet);
        host_a.state = WAITLAYER3;
        starttimer(A, TIMEOUT);
    }
    else
    {
        cout << "Waiting for acknowledgement of the correct packet, hence adding to message buffer" << endl;
        return;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if ((packet.checksum != get_checksum(packet)) || (packet.acknum != host_a.acknum))
    {
        cout << "Packet is corrupted/has wrong ackno, waiting ack no is: " << host_a.acknum << endl;
        return;
    }

    cout << "Right acknowledgement received without corruption and right ack no:" << host_a.acknum << endl;
    stoptimer(A);
    msg_buffer.pop();
    host_a.seqnum = (host_a.seqnum + 1) % 2;
    host_a.acknum = host_a.seqnum;
    host_a.state = WAITLAYER5;

    if (!msg_buffer.empty())
    {
        pkt packet = make_packet(host_a.seqnum, msg_buffer.front());
        tolayer3(A, packet);
        host_a.state = WAITLAYER3;
        starttimer(A, TIMEOUT);
    }
    cout << "Leaving A_input" << endl;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    pkt packet = make_packet(host_a.seqnum, msg_buffer.front());
    tolayer3(A, packet);
    host_a.state = WAITLAYER3;
    starttimer(A, TIMEOUT);
    cout << "Leaving timerinterrupt" << endl;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    host_a.seqnum = 0;
    host_a.acknum = host_a.seqnum;
    host_a.state = WAITLAYER5;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if (packet.checksum == get_checksum(packet))
    {
        pkt ack_packet = make_ACKpacket(packet.acknum);
        tolayer3(B, ack_packet);

        if (packet.acknum == host_b.acknum)
        {
            tolayer5(B, packet.payload);
            host_b.acknum = (host_b.acknum + 1) % 2;
        }
    }
    else
    {
        cout << "packet corruption!" << endl;
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    host_b.acknum = 0;
}