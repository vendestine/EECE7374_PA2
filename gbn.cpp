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

//packet in the window
vector<pkt> unacked(1000);

//message buffer outside window
queue<msg> msg_buffer;

//ACK packet
pkt ack_packet;

//create variables for sender
struct Sender
{
	int base;
	int next_seqnum;
	int N;
}host_a;

//create variables for receiver
struct Receiver
{
	int expected_seqnum;

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
	packet.seqnum = acknum;
	packet.acknum = acknum;
	memset(packet.payload, 0, sizeof(packet.payload));
	packet.checksum = get_checksum(packet);
	return packet;

}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	if (host_a.next_seqnum < host_a.base + host_a.N)
	{
		if (msg_buffer.empty())
		{
			pkt packet = make_packet(host_a.next_seqnum, message);
			unacked[host_a.next_seqnum] = packet;
			tolayer3(A, packet);

			if (host_a.base == host_a.next_seqnum)
				starttimer(A, TIMEOUT);
			host_a.next_seqnum++;

		}

		else
		{
			msg_buffer.push(message);
			msg buffer_message = msg_buffer.front();
			msg_buffer.pop();


			pkt packet = make_packet(host_a.next_seqnum, buffer_message);
			unacked[host_a.next_seqnum] = packet;
			tolayer3(A, packet);

			if (host_a.base == host_a.next_seqnum)
				starttimer(A, TIMEOUT);
			host_a.next_seqnum++;
		}
	}

	else
	{
		msg_buffer.push(message);

	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	if (packet.checksum == get_checksum(packet))
	{
		host_a.base = packet.acknum + 1;
		
		if (host_a.base == host_a.next_seqnum)
			stoptimer(A);
		else
		{
			stoptimer(A);
			starttimer(A, TIMEOUT);
		}
	}

	else
	{
		cout << "packet corruption" << endl;
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	starttimer(A, TIMEOUT);
	for (int i = host_a.base; i < host_a.next_seqnum; i++)
	{
		tolayer3(A, unacked[i % host_a.N]);
	}
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	host_a.base = 1;
	host_a.next_seqnum = 1;
	host_a.N = getwinsize();
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	if (packet.checksum == get_checksum(packet) && packet.seqnum == host_b.expected_seqnum)
	{
		tolayer5(B, packet.payload);
		ack_packet = make_ACKpacket(host_b.expected_seqnum);
		host_b.expected_seqnum++;
	}
	tolayer3(B, ack_packet);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	host_b.expected_seqnum = 1;
	ack_packet = make_ACKpacket(0);
}