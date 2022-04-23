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
#include <cstring>
#include <queue>

using namespace std;

//define host A and B
#define A 0
#define B 1

//define timeout for retransmission
#define TIMEOUT 20.0

//define host A state (wait up layer or wait down layer)
#define WAIT_LAYER3 0
#define WAIT_LAYER5 1

//message buffer
queue<msg> msg_buffer;

//saved packet for possible retransmission
struct pkt resend_packet;

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
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < 20; ++i)
    {
        checksum += packet.payload[i];
    }
    return checksum;
}

//make packet for sender
pkt make_packet(int seqnum, msg message)
{
    pkt packet;
    packet.seqnum = seqnum;
    //excpeted ack number is sequnce number
    packet.acknum = seqnum;
    strncpy(packet.payload, message.data, 20);
    packet.checksum = get_checksum(packet);
    return packet;
}

//make packet for receiver
pkt make_ACKpacket(int acknum)
{
    pkt packet;
    //for ACK packet, we only focus ack number
    packet.seqnum = 0;
    packet.acknum = acknum;
    memset(packet.payload, 0, sizeof(packet.payload));
    packet.checksum = get_checksum(packet);
    return packet;

}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    // if host A waits up layer, send the packet
    if (host_a.state == WAIT_LAYER5)
    {
        pkt packet = make_packet(host_a.seqnum, message);
        //save packet for possible retransimission
        resend_packet = packet;
        tolayer3(A, packet);
        //after send packet to layer 3, change state
        host_a.state = WAIT_LAYER3;
        starttimer(A, TIMEOUT);
    }
    else
    {
        //if host A wait down layer, buffer current message
        msg_buffer.push(message);
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    //receive right ACK packet, no corruption, stop timer
    if ((packet.checksum == get_checksum(packet)) && (packet.acknum == host_a.acknum))
    {
        stoptimer(A);
        //flip sender sequnce number for next packet
        host_a.seqnum = (host_a.seqnum + 1) % 2;
        host_a.acknum = host_a.seqnum;
        //change state to wait up layer
        host_a.state = WAIT_LAYER5;

        //there is meesage in buffer
        if (!msg_buffer.empty())
        {
            //send the message, after sending, pop the message
            pkt packet = make_packet(host_a.seqnum, msg_buffer.front());
            msg_buffer.pop();
            //also save packet for possible retransmission
            resend_packet = packet;
            tolayer3(A, packet);
            //after send packet to layer3, change state
            host_a.state = WAIT_LAYER3;
            starttimer(A, TIMEOUT);
        }
    }

    //there is packet corruption or receive wrong ACK
    else
    {
        return;
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    //resned the packet
    tolayer3(A, resend_packet);
    //after send packet to layer3, change state
    host_a.state = WAIT_LAYER3;
    starttimer(A, TIMEOUT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    host_a.seqnum = 0;
    host_a.acknum = host_a.seqnum;
    //in the beginning, wait for message from up player
    host_a.state = WAIT_LAYER5;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    //packet no corruption
    if (packet.checksum == get_checksum(packet))
    {
        //send ACK packet to A
        pkt ack_packet = make_ACKpacket(packet.acknum);
        tolayer3(B, ack_packet);

        //no receive this packet before, deliver message to up layer
        if (packet.acknum == host_b.acknum)
        {
            tolayer5(B, packet.payload);
            //flip receiver ack number for next packet
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