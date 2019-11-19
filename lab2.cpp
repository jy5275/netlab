/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

extern void ip_DiscardPkt(char* pBuffer, int type);

extern void ip_SendtoLower(char*pBuffer, int length);

extern void ip_SendtoUp(char *pBuffer, int length);

extern unsigned int getIpv4Address();

// implemented by students

struct Ipv4 {
	unsigned IHL : 4;
	unsigned version : 4;
	unsigned serviceType : 6;
	unsigned padding1 : 2;
	unsigned totalLen : 16;
	unsigned id : 16;
	unsigned padding2 : 1;
	unsigned DF : 1;
	unsigned MF : 1;
	unsigned FragOff : 13;
	unsigned timeToLive : 8;
	unsigned protocol : 8;
	unsigned headerChecksum : 16;
	unsigned srcAddr : 32;
	unsigned dstAddr : 32;
};

int stud_ip_recv(char *pBuffer, unsigned short length) {
	Ipv4 *ipv4 = (Ipv4*)pBuffer;
	if (ipv4->version != 4) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
		return 1;
	}
	if (ipv4->IHL < 5) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
		return 1;
	}
	if ((int)(ipv4->timeToLive) == 0) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
		return 1;
	}
	unsigned dstAddr = ntohl(ipv4->dstAddr);
	unsigned myip = getIpv4Address();
	if (dstAddr != myip && dstAddr != 0xffffffff) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
		return 1;
	}
	int checksum = ntohs(ipv4->headerChecksum);

	short *p = (short*)pBuffer;
	int headlen = ipv4->IHL * 4;
	int sum = 0;
	for (int i = 0; i < headlen; i += 2, p++) {
		sum += ntohs(*p);
	}
	if ((~sum) & 0xffff != checksum) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}
	ip_SendtoUp(pBuffer, length);
	return 0;
}


int stud_ip_Upsend(char *pBuffer, unsigned short len, unsigned int srcAddr,
	unsigned int dstAddr, byte protocol, byte ttl) {


	ip_SendtoLower(pBuffer, len);
	return 0;
}