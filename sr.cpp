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

#include<vector>
#include<queue>
#include<unordered_map>
#include<iostream>
#include<stdio.h>
#include<string.h>

using namespace std;

struct Sender
{
  int send_base;
  int seqnum;
  int acknum;
  int next_seqnum;
}host_a;

struct Receiver
{
  int rcv_base;
  int next_seqnum;
}host_b;


int winSize = 0;
float RTT = 100.0;

//initial a buffer for side A that stores unsent message
vector<msg> msgBufferA;
//initialize a buffer for side B that stores packets' seq number and the packets (not yet send up to layer 5)
vector<pair<int, pkt>> msgBufferB;


//two hash maps, one stores the packet seq number and the time it was sent. Another keeps track of whether or not if a packet was acked.
unordered_map<int, float> timeStamps;
unordered_map<int, bool> acked;

//using a queue to store the order of packet's seq number 
queue<int> pacTimeOrder;


/* To calculate checkSum for packet*/
int checkSum(struct pkt packet) {
  int sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += packet.payload[i];
  }
  sum += packet.acknum;
  sum += packet.seqnum;

  return sum;
}

/* Sending packet to layer 3*/
void send_packet(int flag, struct pkt packet) {
  tolayer3(flag, packet);
}

//send packet at side A
void send_nextpacket(int flag) {
  //while there is message left in side A's buffer, keep sending
  while (host_a.next_seqnum - host_a.send_base < winSize && host_a.next_seqnum < msgBufferA.size()) {
    //construct the packet
    struct pkt packet;
    packet.seqnum = host_a.next_seqnum;
    packet.acknum = host_a.acknum;
    strncpy(packet.payload, msgBufferA[host_a.next_seqnum].data, 20);
    packet.checksum = checkSum(packet);
    cout << "Sending packet seq -> " << packet.seqnum << endl;
    send_packet(flag, packet);
    //store the time that a packet was sent
    timeStamps[host_a.next_seqnum] = get_sim_time();
    //initialize the ack status of the seq num to be false
    acked[host_a.next_seqnum] = false;
    //push the packet's seq num to the front of the queue so that it will be popped first
    pacTimeOrder.push(host_a.next_seqnum);
    host_a.next_seqnum += 1;

    //if there is only one packet in the packet order array, start the timer. Since there is no need to start timer when its 0
    //and the timer would already be started if its bigger than 1.
    if (pacTimeOrder.size() == 1) {
      // To start A's Timer
      cout << " ########################################################TIMER STARTED FOR A WITH SIM TIME >>>> " << RTT << endl;
      starttimer(flag, RTT);
    }
  }
}

//function that returns the position of deesired receiver base in side B's buffer, return -1 if not found
int findRcvBase() {
  for (int i = 0; i < msgBufferB.size(); i++) {
    if (msgBufferB[i].first == host_b.rcv_base) {
      return i;
    }
  }
  return -1;
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  msgBufferA.push_back(message);
  //if its within the alloed range, then send, otherwise just store in the buffer
  if (host_a.next_seqnum < host_a.send_base + winSize) {
    cout << "sending" << endl;
    send_nextpacket(0);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  int resCheckSum = checkSum(packet);
  if (resCheckSum == packet.checksum) {
    cout << "ack checksum verified" << endl;
    //mark the packet's seq num as acked
    acked[packet.acknum] = true;
    if (packet.acknum == host_a.send_base) {
      //advance window base to nextr unACKed seq number
      while (host_a.send_base < msgBufferA.size() && acked[host_a.send_base] == true){
        host_a.send_base++;
      }
      send_nextpacket(0);
    }

    //expecting the packet with seq number in the front of packet order queue
    int expectedPacket = pacTimeOrder.front();
    if (packet.acknum == expectedPacket) {
      //if received expected ack, then stop timer
      stoptimer(0);
      cout << "expected ack received, timer stopped";
      //pop all ACKed packet seq number
      while (pacTimeOrder.size() > 0 && acked[pacTimeOrder.front()] == true) {
        pacTimeOrder.pop();
      }
      //get the start time of the next unACKed packet
      float startTime = timeStamps[pacTimeOrder.front()];
      if (pacTimeOrder.size() > 0) {
        cout << "Timer started for the next packet" << endl;
        //then start timer with the set time minus the time elapsed since the packet was sent
        //the time elapsed is obtaineed by minusing the time right now to the time it was sent out.
        starttimer(0, RTT - (get_sim_time() - startTime));
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  cout << "timer stopped";
  //get the seq number of the packet that was interruppted
  int packetLost = pacTimeOrder.front();
  //remake the packet
  struct pkt packet;
  packet.seqnum = packetLost;
  packet.acknum = host_a.acknum;
  strncpy(packet.payload, msgBufferA[packetLost].data, 20);
  packet.checksum = checkSum(packet);

  //pop that packet's seq number from the packet order queue
  pacTimeOrder.pop();

  //advance to the next unACKed packet
  while (pacTimeOrder.size() > 0 && acked[pacTimeOrder.front()]) {
    pacTimeOrder.pop();
  }
  //start timer again, similar to A_input
  float startTime = timeStamps[pacTimeOrder.front()];
  if (pacTimeOrder.size() > 0) {
    starttimer(0, RTT - (get_sim_time() - startTime));
    cout << "Timer started";
  }
  //re-push the packet seq number into the queue and get the new time it was sent out
  pacTimeOrder.push(packetLost);
  timeStamps[packetLost] = get_sim_time();
  send_packet(0, packet);
  
  //start timer if the interuppted packet is the only one left in queue
  if (pacTimeOrder.size() == 1) {
    starttimer(0, RTT);
  }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  host_a.send_base = 0;
  host_a.next_seqnum = 0;
  host_a.acknum = 0;
  host_a.seqnum = 0;
  winSize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  int resCheckSum = checkSum(packet);
  if (resCheckSum == packet.checksum) {
    cout << "Checksum verified" << endl;

    //send ack packet
    struct pkt ackPacket;
    ackPacket.seqnum = 0;
    ackPacket.acknum = packet.seqnum;
    memset(ackPacket.payload, 0, 20);
    ackPacket.checksum = checkSum(ackPacket);

    send_packet(1, ackPacket);
    cout << "ACK sent" << endl;
    //send to layer 5 if the packet seq number is as expected, otherwise buffer
    if (host_b.rcv_base == packet.seqnum) {
      tolayer5(1, packet.payload);
      host_b.rcv_base++;
    } else if (packet.seqnum >= host_b.rcv_base && packet.seqnum <= host_b.rcv_base + winSize - 1) {
      msgBufferB.push_back({packet.seqnum, packet});
    }
  }

  //find if the updated expected seq number is in the buffer. If so, deliver the buffered packets
  int basePos = findRcvBase();
  while (msgBufferB.size() > 0 && basePos != -1) {
    char data[20];
    strncpy(data, msgBufferB[basePos].second.payload, 20);
    tolayer5(1, data);
    host_b.rcv_base += 1;
    basePos = findRcvBase();
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  host_b.rcv_base = 0;
}
