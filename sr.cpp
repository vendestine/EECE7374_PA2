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
#define TIMEOUT 100.0

//create variables for sender
struct Sender
{
	//window start position
	int send_base;
	//pakcet sequence number
	int next_seqnum;
}host_a;

//create variables for receiver
struct Receiver
{
	int rcv_base;
}host_b;

struct pkt_time_flag
{
	pkt packet;
	float send_time;

	bool is_sent;
	bool is_acked;
};

//sender work packets
vector<pkt_time_flag> sender_packets(1000);

//receiver work packets
vector<pkt> receiver_packets(1000);

//message buffer outside window
queue<msg> msg_buffer;




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
pkt make_packet(int seqnum, int acknum, msg message)
{
	pkt packet;
	packet.seqnum = seqnum;
	//excpeted ack number is sequnce number
	packet.acknum = acknum;
	strncpy(packet.payload, message.data, 20);
	packet.checksum = get_checksum(packet);
	return packet;
}

pkt_time_flag make_data_packet(pkt packet)
{
	pkt_time_flag data_packet;
	data_packet.packet = packet;
	data_packet.send_time = get_sim_time();
	return data_packet;
}

////make packet for receiver
//pkt make_ACKpacket(int seqnum, int acknum)
//{
//	pkt packet;
//	//for ACK packet, we only focus ack number
//	packet.seqnum = seqnum;
//	packet.acknum = acknum;
//	memset(packet.payload, 0, sizeof(packet.payload));
//	packet.checksum = get_checksum(packet);
//	return packet;
//}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	if (host_a.next_seqnum < host_a.send_base + getwinsize())
	{
		if (msg_buffer.empty())
		{
			pkt packet = make_packet(host_a.next_seqnum, -1, message);
			pkt_time_flag data_packet = make_data_packet(packet);
			sender_packets[host_a.next_seqnum] = data_packet;
			data_packet.is_sent = true;
			tolayer3(A, packet);
			starttimer(A, TIMEOUT);
			host_a.next_seqnum++;
		}

		else
		{
			msg_buffer.push(message);
			msg buffer_message = msg_buffer.front();
			msg_buffer.pop();

			pkt packet = make_packet(host_a.next_seqnum, -1, message);
			pkt_time_flag data_packet = make_data_packet(packet);
			sender_packets[host_a.next_seqnum] = data_packet;
			data_packet.is_sent = true;
			tolayer3(A, packet);
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
		sender_packets[packet.seqnum].is_acked = true;

		if (packet.seqnum == host_a.send_base)
		{
			while (!sender_packets[host_a.send_base].is_acked) host_a.send_base++;

			stoptimer(A);

			for (int i = host_a.send_base; i < host_a.next_seqnum; i++)
			{
				if (!sender_packets[i].is_sent)
				{
					sender_packets[i].is_sent = true;
					sender_packets[i].send_time = get_sim_time();
					tolayer3(A, sender_packets[i].packet);
					starttimer(A, TIMEOUT);
				}
			}
		}

		else
		{
			starttimer(A, TIMEOUT);
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	if (host_a.send_base < host_a.next_seqnum)
	{
		for (int i = host_a.send_base; i < host_a.next_seqnum; i++)
		{
			float wait_time = get_sim_time() - sender_packets[i].send_time;
			if (wait_time >= TIMEOUT && !sender_packets[i].is_acked)
			{
				sender_packets[i].send_time = get_sim_time();
				tolayer3(A, sender_packets[i].packet);
				starttimer(A, TIMEOUT);
			}
		}
	}
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	host_a.send_base = 0;
	host_a.next_seqnum = 0;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	
	msg message;
	strncpy(message.data, packet.payload, 20);

	if (packet.checksum == get_checksum(packet))
	{
		if (packet.seqnum >= host_b.rcv_base && packet.seqnum <= host_b.rcv_base + getwinsize() - 1)
		{
			if (receiver_packets[packet.seqnum].seqnum = -1)
			{
				pkt ack_packet = make_packet(packet.seqnum, packet.seqnum, message);
				//strncpy(ack_packet.payload, packet.payload, 20);
				receiver_packets[packet.seqnum] = ack_packet;
				tolayer3(B, ack_packet);
			}

			if (host_b.rcv_base == packet.seqnum)
			{
				for (int i = host_b.rcv_base; i <= host_b.rcv_base + getwinsize() - 1; i++)
				{
					if (receiver_packets[i].acknum != -1)
					{
						tolayer5(B, receiver_packets[i].payload);
						host_b.rcv_base++;
					}
					else break;
				}
			}
		}


		else if (packet.seqnum >= host_b.rcv_base - getwinsize() && packet.seqnum <= host_b.rcv_base - 1)
		{
			pkt ack_packet = make_packet(packet.seqnum, packet.seqnum, message);
			//strncpy(ack_packet.payload, packet.payload, 20);
			tolayer3(B, ack_packet);
		}
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	host_b.rcv_base = 0;

	for (int i = 0; i < 1000; i++)
	{
		receiver_packets[i].seqnum = -1;
		receiver_packets[i].acknum = -1;
	}
}