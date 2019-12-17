/* THIS FILE IS FOR TCP TEST */
/*
struct sockaddr_in {
short   sin_family;
u_short sin_port;
struct  in_addr sin_addr;
char    sin_zero[8];
};
*/
#include "sysInclude.h"

extern void tcp_DiscardPkt(char *pBuffer, int type);
extern void tcp_sendReport(int type);
extern void tcp_sendIpPkt(unsigned char *pData, UINT16 len, unsigned int srcAddr,
	unsigned int dstAddr, UINT8 ttl);
extern int waitIpPacket(char *pBuffer, int timeout);
extern unsigned int getIpv4Address();
extern unsigned int getServedipv4Address();

typedef int STATE;
enum TCP_STATES { CLOSED, SYN_SENT, ESTABLISHED, FIN_WAIT1, FIN_WAIT2, TIME_WAIT };
const unsigned int BUFLEN = 1024;
const int WAITTIME = 5000;
const unsigned char TLL = 128;
const unsigned int MAX_TCP_CONNECTIONS = 128;

struct TCPHead {
	unsigned short sport;
	unsigned short dport;
	unsigned int seq;
	unsigned int ack;
	UINT8 hlen;
	UINT8 flags;
	unsigned short winsize;
	unsigned short checksum;
	unsigned short purg;
	char data[BUFLEN];
};

struct MyTCB {
	STATE state;
	unsigned int sip, dip;
	unsigned short sport, dport;
	unsigned int seq;	// 1st byte seq in the next packet sent here
	unsigned int ack;	// Our next expected byte (set after receiving a pkt)
	unsigned char flags;
	bool used;
};
struct PseudoHead {
	unsigned sip;
	unsigned dip;
	UINT8 reserve;
	UINT8 protocol;
	unsigned short TCPlen;
};
struct MyTCB tcb[MAX_TCP_CONNECTIONS];
int gsockfd = -1;
int gSrcPort = 2005;
int gDstPort = 2006;
int gSeqNum = 1;
int gAckNum = 1;

// Final ans host sequence
unsigned short calcCheckSum(TCPHead *head, unsigned len, unsigned sip, unsigned dip) {
	unsigned sum = 0, temp = 0;
	int reallen = len % 2 ? len + 1 : len;
	for (int i = 0; i < 10 + reallen / 2; i++) {
		temp = ntohs(((unsigned short*)head)[i]);
		sum = (sum + temp) % 0xffff;
	}
	// PseudoHead
	struct PseudoHead pseudohead;
	pseudohead.sip = htonl(sip); pseudohead.dip = htonl(dip); pseudohead.reserve = 0;
	pseudohead.protocol = IPPROTO_TCP; pseudohead.TCPlen = htons(20 + len);
	for (int i = 0; i < 6; i++) {
		temp = ntohs(((unsigned short*)(&pseudohead))[i]);
		sum = (sum + temp) % 0xffff;
	}
	return (~sum) & 0xffff;
}

int stud_tcp_input(char *pBuffer, unsigned short len, unsigned int srcAddr,
	unsigned int dstAddr) {
	struct TCPHead recv = *((struct TCPHead*)pBuffer);/*
													  if (calcCheckSum(&recv, len - 20, ntohl(srcAddr), ntohl(dstAddr)) != 0xffff) {
													  printf("stud_tcp_input: checksum error\n");
													  return -1;
													  }*/
	recv.sport = ntohs(recv.sport);
	recv.dport = ntohs(recv.dport);
	recv.seq = ntohl(recv.seq);
	recv.ack = ntohl(recv.ack);
	recv.winsize = ntohs(recv.winsize);
	recv.purg = ntohs(recv.purg);

	if (recv.seq != tcb[gsockfd].ack) {
		tcp_DiscardPkt(pBuffer, STUD_TCP_TEST_SEQNO_ERROR);
		tcp_sendReport(STUD_TCP_TEST_SEQNO_ERROR);
		printf("stud_tcp_input: seq num error\n");
		return -1;
	}

	switch (tcb[gsockfd].state) {
	case SYN_SENT:	// Change state to ESTABLISHED
		if (recv.flags == PACKET_TYPE_SYN_ACK) {
			tcb[gsockfd].state = ESTABLISHED;
			// No data payload in ACK packet
			// So we still expect the (seq+1)th byte next time
			tcb[gsockfd].ack = recv.seq + 1;
			tcb[gsockfd].seq = recv.ack;	// Server want ack'th byte from us
			stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb[gsockfd].sport,
				tcb[gsockfd].dport, tcb[gsockfd].sip, tcb[gsockfd].dip);
		} break;
	case ESTABLISHED:
		if (recv.flags == PACKET_TYPE_ACK) {
			tcb[gsockfd].ack = recv.seq + (len - 20);
			tcb[gsockfd].seq = recv.ack;
		}
		else if (recv.flags == PACKET_TYPE_DATA) {
			tcb[gsockfd].ack = recv.seq + (len - 20);
		}
		break;
	case FIN_WAIT1:
		if (recv.flags == PACKET_TYPE_ACK) {
			tcb[gsockfd].state = FIN_WAIT2;
			// No data payload in ACK packet
			// So we still expect the (seq+1)th byte next time
			// tcb[gsockfd].ack = recv.seq + 1;
			tcb[gsockfd].seq = recv.ack;	// Server want ack'th byte from us
		} break;
	case FIN_WAIT2:
		if (recv.flags == PACKET_TYPE_FIN_ACK) {
			tcb[gsockfd].state = TIME_WAIT;
			tcb[gsockfd].ack = recv.seq + 1;
			stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb[gsockfd].sport,
				tcb[gsockfd].dport, tcb[gsockfd].sip, tcb[gsockfd].dip);
		} break;
	default:
		printf("stud_tcp_input: Unknown gsockfd state\n");
		return -1;
	}

	return 0;
}


void stud_tcp_output(char *pData, unsigned short len, unsigned char flag,
	unsigned short srcPort, unsigned short dstPort, unsigned int srcAddr, unsigned int dstAddr)
{
	struct TCPHead packet;
	memcpy(packet.data, pData, len);

	packet.sport = htons(srcPort);
	packet.dport = htons(dstPort);
	if (gsockfd == -1) {
		gsockfd = 0;
		tcb[0].state = SYN_SENT;
		tcb[0].ack = gAckNum;
		tcb[0].seq = gSeqNum;
		tcb[0].sport = gSrcPort++;
		tcb[0].dport = dstPort;
		tcb[0].dip = dstAddr;
		tcb[0].sip = getIpv4Address();

		packet.seq = htonl(gSeqNum);
		packet.ack = htonl(gAckNum);
	}
	else {
		packet.seq = htonl(tcb[gsockfd].seq);
		packet.ack = htonl(tcb[gsockfd].ack);
	}
	packet.hlen = 0x50;
	packet.flags = flag;
	packet.winsize = htons(1);
	packet.checksum = htons(0);
	packet.purg = htons(0);
	if (tcb[gsockfd].state == ESTABLISHED && flag == PACKET_TYPE_FIN_ACK) {
		tcb[gsockfd].state = FIN_WAIT1;
	}

	packet.checksum = htons(calcCheckSum(&packet, len, srcAddr, dstAddr));
	tcp_sendIpPkt((unsigned char*)(&packet), len + 20, srcAddr, dstAddr, TLL);	// ???????��?????
}


int stud_tcp_socket(int domain, int type, int protocol) {
	if (domain != AF_INET || type != SOCK_STREAM || protocol != IPPROTO_TCP)
		return -1;
	for (int i = 5; i < MAX_TCP_CONNECTIONS; i++) {
		struct MyTCB *p = &tcb[i];
		if (p->used)
			continue;
		p->used = true;
		p->sport = gSrcPort++;
		p->seq = gSeqNum++;
		p->ack = gAckNum;
		p->state = CLOSED;
		return i;
	}
	printf("stud_tcp_socket: no socket slot available\n");
	return -1;
}


int stud_tcp_connect(int sockfd, struct sockaddr_in *addr, int addrlen) {
	int len;
	if (sockfd > MAX_TCP_CONNECTIONS) {
		printf("stud_tcp_connect: Bad sockfd num\n");
		return -1;
	}
	struct MyTCB *p = &tcb[sockfd];
	if (!p->used) {
		printf("stud_tcp_connect: sockfd not exist\n");
		return -1;
	}

	p->sip = getIpv4Address();
	p->dip = ntohl(addr->sin_addr.s_addr);	// ???
	p->dport = ntohs(addr->sin_port);	// ???
	p->state = SYN_SENT;
	gsockfd = sockfd;
	stud_tcp_output(NULL, 0, PACKET_TYPE_SYN, p->sport, p->dport, p->sip, p->dip);

	char buffer[BUFLEN];
	len = -1;
	while (len == -1)
		len = waitIpPacket(buffer, WAITTIME);	// Waiting for SYN-ACK

	if (len < 20) {
		printf("stud_tcp_connect: Error TCP packet length\n");
		return -1;
	}
	stud_tcp_input(buffer, 20, p->sip, p->dip);
	return 0;
}


int stud_tcp_send(int sockfd, const unsigned char *pData, unsigned short datalen, int flags) {
	int len;
	struct MyTCB *p = &tcb[sockfd];
	if (p->state != ESTABLISHED) {
		printf("stud_tcp_send: Connection not established\n");
		return -1;
	}
	gsockfd = sockfd;
	stud_tcp_output((char*)pData, datalen, PACKET_TYPE_DATA, p->sport, p->dport, p->sip, p->dip);	// flags???

	char buffer[BUFLEN];
	len = waitIpPacket(buffer, WAITTIME);	// Waiting for ACK
	if (len < 20) {
		printf("stud_tcp_send: Error TCP packet length\n");
		return -1;
	}
	stud_tcp_input(buffer, len, p->dip, p->sip);
	return 0;
}


int stud_tcp_recv(int sockfd, unsigned char *pData, unsigned short datalen, int flags) {
	int len;
	struct MyTCB *p = &tcb[sockfd];
	if (p->state != ESTABLISHED) {
		printf("stud_tcp_recv: Connection not established\n");
		return -1;
	}
	gsockfd = sockfd;
	char buffer[BUFLEN];
	len = waitIpPacket(buffer, WAITTIME);
	if (len < 20) {
		printf("stud_tcp_recv: Error TCP packet length\n");
		return -1;
	}
	memcpy(pData, ((struct TCPHead*)(buffer))->data, datalen);
	stud_tcp_input(buffer, len, p->sip, p->dip);
	stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, p->sport, p->dport, p->sip, p->dip);
	return 0;
}


int stud_tcp_close(int sockfd) {
	int len;
	struct MyTCB *p = &tcb[sockfd];
	gsockfd = sockfd;
	switch (p->state) {
	case ESTABLISHED:
		p->state = FIN_WAIT1;
		stud_tcp_output(NULL, 0, PACKET_TYPE_FIN_ACK, p->sport, p->dport, p->sip, p->dip);
		char buffer[BUFLEN];
		len = waitIpPacket(buffer, WAITTIME);	// Waiting for ACK pkg
		stud_tcp_input(buffer, len, p->sip, p->dip);	// Handin ACK pkg
		len = waitIpPacket(buffer, WAITTIME);	// Waiting for FIN pkg
		stud_tcp_input(buffer, len, p->sip, p->dip);	// Handin FIN pkg
		tcb[sockfd].used = false;
		break;
	default:
		tcb[sockfd].used = false;
		break;
	}
	return 0;
}
