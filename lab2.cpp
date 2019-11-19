/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

extern void ip_DiscardPkt(char* pBuffer, int type);

extern void ip_SendtoLower(char* pBuffer, int length);

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
	//unsigned padding2 : 1;
	//unsigned DF : 1;
	//unsigned MF : 1;
	unsigned FragOff : 16;
	unsigned timeToLive : 8;
	unsigned protocol : 8;
	unsigned headerChecksum : 16;
	unsigned srcAddr : 32;
	unsigned dstAddr : 32;

	Ipv4(int _len, int _ttl, int _protocol, int _src, int _dst) {
		IHL = 5;
		version = 4;
		serviceType = 0; padding1 = 0; id = 0;
		FragOff = 0; headerChecksum = 0;
		totalLen = htons(_len + 20);
		timeToLive = _ttl;
		protocol = _protocol;
		srcAddr = htonl(_src);
		dstAddr = htonl(_dst);

		unsigned short sum = 0, temp = 0;
		for (int i = 0; i < 10; i++) {
			temp = (((unsigned char*)this)[i * 2] << 8) + ((unsigned char*)this)[i * 2 + 1];
			if (sum + temp > 0xffff)
				sum += (temp + 1);
			else
				sum += temp;
		}
		headerChecksum = htons(0xffff - sum);
	}
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

	unsigned short sum = 0, temp = 0;
	for (int i = 0; i < ipv4->IHL * 2; i++) {
		temp = (((unsigned char*)pBuffer)[2*i] << 8) + ((unsigned char*)pBuffer)[2*i + 1];
		if (sum + temp > 0xffff)
			sum += (temp + 1);
		else
			sum += temp;
	}
	if (sum != 0xffff) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}

	ip_SendtoUp(pBuffer, length);
	return 0;
}


int stud_ip_Upsend(char *pBuffer, unsigned short len, unsigned int srcAddr,
	unsigned int dstAddr, byte protocol, byte ttl) {
	struct Ipv4 *ipv4 = new Ipv4(len, ttl, protocol, srcAddr, dstAddr);

	char *package = new char[len + 20];
	memcpy(package + 20, pBuffer, len);
	memcpy(package, ipv4, 20);

	ip_SendtoLower(package, len + 20);
	delete ipv4;
	return 0;
}