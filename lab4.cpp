/* THIS FILE IS FOR TCP TEST */

struct sockaddr_in {
	short   sin_family;
	u_short sin_port;
	struct  in_addr sin_addr;
	char    sin_zero[8];
};

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
const unsigned int BUFLEN = 2048;
const int WAITTIME = 5000;
const unsigned char TLL = 128;

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
	unsigned int seq, ack;
	unsigned char flags;
	bool used;
	//int data_ack;
	//unsigned char data[BUFLEN];
	//unsigned short data_len;
};
struct PseudoHead{
	unsigned sip;
	unsigned dip;
	UINT8 reserve;
	UINT8 protocol;
	unsigned short TCPlen;
	PseudoHead(unsigned _sip, unsigned _dip, unsigned short _len) {
		sip = htonl(_sip); dip = htonl(_dip); reserve = 0;
		protocol = IPROTO_TCP; TCPlen = htons(_len);
	}
}
struct MyTCB tcb[MAX_TCP_CONNECTIONS];
int gsockfd;
int gSrcPort = 2005;
int gDstPort = 2006;
int gSeqNum = 0;
int gAckNum = 0;
int socknum = 5;

unsigned short calcCheckSum(TCPHead *head, unsigned len, unsigned sip, unsigned dip) {
	unsigned sum = 0, temp = 0;
	int reallen = len%2 ? len+1 : len;
	for (int i = 0; i < 10 + reallen/2; i++) {
		temp = ntohs(((unsigned short*)head)[i]);
		sum = (sum + temp) % 0xffff;
	}
	// PsudoHead
	struct PsedoHead psedohead(sip, dip, 20 + len);
	for (int i = 0; i < 6; i++) {
		temp = ntohs(((unsigned short*)(&psedohead))[i]);
		sum = (sum + temp) % 0xffff;
	}
	sum = htons(0xffff - sum);
}

int stud_tcp_input(char *pBuffer, unsigned short len, unsigned int srcAddr, 
	unsigned int dstAddr) {
	struct TCPHead tcp_seg;
	if (len < 20) return -1;
	memcpy(&tcp_seg, pBuffer, len);

	return 0;
}


void stud_tcp_output(char *pData, unsigned short len, unsigned char flag, 
	unsigned short srcPort, unsigned short dstPort, unsigned int srcAddr, unsigned int dstAddr)
{
	struct TCPHead packet;
	memcpy(packet.data, pData, len);
	
	packet.sport = htons(srcPort);
	packet.dport = htons(dstPort);
	packet.seq = htonl(tcb[gsockfd].seq);
	packet.ack = htonl(tcb[gsockfd].ack);
	packet.hlen = 0x50;
	packet.flags = flag;
	packet.winsize = htonl(1);
	packet.purg = 0;
	packet.checksum = 0;

	packet.checksum = calcCheckSum(&packet, len, srcAddr, dstAddr);
	tcp_sendIpPkt((unsigned char*)(&packet), len + 20, srcAddr, dstAddr, TLL);	// ×Ö½ÚÐò???
}


int stud_tcp_socket(int domain, int type, int protocol) {
	if(domain != AF_INET || type != SOCK_STREAM || protocol != IPPROTO_TCP)
		return -1;
	for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
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
	if (sockfd > MAX_TCP_CONNECTIONS)
		return -1;
	struct MyTCB *p = &tcb[sockfd];
	if (!p->used) 
		return -1;
	
	p->sip = getIpv4Address();
	p->dip = ntohl(addr->sin_addr.s_addr);
	p->dport = ntohl(addr->sin_port);	// ???
	p->state = SYN_SENT;
	stud_tcp_output(NULL, 0, PACKET_TYPE_SYN, p->sport, p->dport, p->sip, p->dip);

	char buffer[2048];
	waitIpPacket(buffer, WAITTIME);
	stud_tcp_input(buffer, 20, p->sip, p->dip);
	return 0;
}


int stud_tcp_send(int sockfd, const unsigned char *pData, unsigned short datalen, int flags){
	int len;
	struct MyTCB *p = &(tcb[sockfd]);
	if (p->state != ESTABLISHED)
		return -1;

	char *pBuffer = new char[2048];
	stud_tcp_output(pData, datalen, flags, p->sport, p->dport, p->sip, p->dip);
	len = waitIpPacket(pBuffer, 1);
	if (len < 20) 
		return -1;
	stud_tcp_input(pBuffer, len, htonl(p->dip), htonl(p->sip));

	delete[] pBuffer;
	return 0;
}


int stud_tcp_recv(int sockfd, unsigned char *pData, unsigned short datalen, int flags) {
	
	return 0;
}


int stud_tcp_close(int sockfd) {
	
	return 0;
}
