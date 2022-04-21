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

std::queue<msg> msg_buffer;

struct Sender
{
    int seqnum;
    int acknum;
    int state;
}host_a;

struct Receiver
{
    int acknum;
}host_b;

int get_checksum(struct pkt packet)
{
    cout << "Entering checksum" << endl;
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < 20; ++i)
    {
        checksum += packet.payload[i];
    }
    cout << "Leaving checksum" << endl;
    return checksum;
}

/*Create a new packet*/
pkt make_packet(int seqnum, int acknum, msg message) {
    pkt packet;
    packet.seqnum = seqnum;
    packet.acknum = acknum;
    strncpy(packet.payload, message.data, 20);
    packet.checksum = get_checksum(packet);
    return packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    msg_buffer.push(message);
    if (host_a.state == WAITLAYER3)
    {
        cout << "Waiting for acknowledgement of the correct packet, hence adding to message buffer" << endl;
        return;
    }
    else
    {
        cout << "Sending the current message" << endl;
        //pkt packet;
        //packet.seqnum = host_a.seqnum;
        //packet.acknum = host_a.acknum;
        //memcpy(packet.payload, message.data, sizeof(message.data));
        //packet.checksum = get_checksum(packet);
        pkt packet = make_packet(host_a.seqnum, host_a.acknum, message);
        cout << "Sending from A:\n" << "msg:" << message.data << "\n" << "Seq Number:" << host_a.seqnum << "\nAck number:" << host_a.acknum << endl;
        tolayer3(A, packet);
        host_a.state = WAITLAYER3;
        starttimer(A, TIMEOUT);
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    cout << "Ack packet received from B with ack number:" << packet.acknum << endl;
    if ((packet.checksum != get_checksum(packet)) || (packet.acknum != host_a.acknum))
    {
        cout << "Packet is corrupted/has wrong ackno, waiting ack no is: " << host_a.acknum << endl;
        return;
    }

    cout << "Right acknowledgement received without corruption and right ack no:" << host_a.acknum << endl;
    stoptimer(A);
    msg_buffer.pop();
    host_a.state = WAITLAYER5;
    host_a.seqnum = (host_a.seqnum + 1) % 2;
    host_a.acknum = (host_a.acknum + 1) % 2;
    if (msg_buffer.size() > 0)
    {
        cout << "Messages has been queued in Buffer, sending that now" << endl;
        //pkt packet;
        //packet.seqnum = host_a.seqnum;
        //packet.acknum = host_a.acknum;
        //string msg = msg_buffer.front();
        //strncpy(packet.payload, msg.c_str(), 20);
        //packet.checksum = get_checksum(packet);
        pkt packet = make_packet(host_a.seqnum, host_a.acknum, msg_buffer.front());
        cout << "Sending from A:\n" << "msg:" << msg_buffer.front().data << "\n" << "Seq Number:" << host_a.seqnum << "\nAck number:" << host_a.acknum << endl;
        tolayer3(A, packet);
        host_a.state = WAITLAYER3;
        starttimer(A, TIMEOUT);
    }
    cout << "Leaving A_input" << endl;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    cout << "Timer has expired for the current packet with Seq number:" << host_a.seqnum << " Hence resending the packet" << endl;
    //pkt packet;
    //packet.seqnum = host_a.seqnum;
    //packet.acknum = host_a.acknum;
    //string msg = msg_buffer.front();
    //strncpy(packet.payload, msg.c_str(), 20);
    //packet.checksum = get_checksum(packet);
    pkt packet = make_packet(host_a.seqnum, host_a.acknum, msg_buffer.front());
    cout << "Sending from A:\n" << "msg:" << packet.payload << "\n" << "Seq Number:" << host_a.seqnum << "\nAck number:" << host_a.acknum << endl;
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
    host_a.acknum = 0;
    host_a.state = WAITLAYER5;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if (packet.checksum == get_checksum(packet))
    {
        cout << "Received packet with payload " << packet.payload << ", seq=" << packet.seqnum << " ack=" << packet.acknum << endl;
        pkt sendPacket;
        if (packet.seqnum == host_b.acknum)
        {
            tolayer5(B, packet.payload);
            sendPacket.acknum = packet.seqnum;
            sendPacket.checksum = get_checksum(sendPacket);
            tolayer3(B, sendPacket);
            host_b.acknum = (host_b.acknum + 1) % 2;
            cout << "Send the ack packet: " << "ack=" << sendPacket.acknum << " next expected seq number:" << host_b.acknum << endl;
        }
        else {
            sendPacket.acknum = packet.seqnum;
            sendPacket.checksum = get_checksum(sendPacket);
            cout << "Response to sender: " << "ack=" << sendPacket.acknum << " next expected seq=" << host_b.acknum << endl;
            tolayer3(B, sendPacket);
        }
    }
    else
    {
        cout << "Packet received is corrupted!" << endl;
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    host_b.acknum = 0;
}