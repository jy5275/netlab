/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <map>
#include <vector>

// system support
extern void fwd_LocalRcv(char *pBuffer, int length);
extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);
extern void fwd_DiscardPkt(char *pBuffer, int type);
extern unsigned int getIpv4Address( );

// implemented by students
struct Ipv4 {
	unsigned IHL : 4;
	unsigned version : 4;
	unsigned serviceType : 6;
	unsigned padding1 : 2;
	unsigned totalLen : 16;
	unsigned id : 16;
	unsigned FragOff : 16;
	unsigned timeToLive : 8;
	unsigned protocol : 8;
	unsigned headerChecksum : 16;
	unsigned srcAddr : 32;
	unsigned dstAddr : 32;
};

typedef struct stud_route_msg {
	unsigned int dest;
	unsigned int masklen;
	unsigned int nexthop;
};
vector<stud_route_msg> table;

void stud_Route_Init() {
	table.clear();
	return;
}


void stud_route_add(stud_route_msg *proute) {
	stud_route_msg newmsg;
	newmsg.masklen = ntohl(proute->masklen);
	int mask = ~((1 << (32 - p->masklen)) - 1);
	newmsg.dest = ntohl(proute->dest) & mask;
	newmsg.nexthop = ntohl(proute->nexthop);
	table.push_back(newmsg);
	return;
}

int find_route(unsigned dip){
	int sublenmax = 0, ret = -1;
	for (int i = 0; i < table.size(); i++) {
		unsigned mask = ~((1 << (32 - table[i].masklen)) - 1);
		if ((table[i].masklen > sublenmax) && (table[i].dest == dip & mask)) {
			ret = i;
			sublenmax = table[i].masklen;
		}
	}
	return ret;
}


int stud_fwd_deal(char *pBuffer, int length) {
	Ipv4 *ipv4 = (Ipv4*)pBuffer;

	unsigned myip = getIpv4Address();
	unsigned destip = ntohl(ipv4->dstAddr);
	if (myip == destip) {
		fwd_LocalRcv(pBuffer, length);
		return 0;
	}
	if (ipv4->timeToLive == 0) {
		fwd_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
		return 1;
	}

	int id = find_route(destip);
	if (id == -1) {
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
		return 1;
	}
	ipv4->timeToLive--;

	ipv4->headerChecksum = 0;
	unsigned sum = 0, temp = 0;
	for (int i = 0; i < 10; i++) {
		temp = ntohs(((unsigned short*)ipv4)[i]);
		sum = (sum + temp) % 0xffff;
	}
	ipv4->headerChecksum = htons(0xffff - sum);

	fwd_SendtoLower(pBuffer, length, table[id].nexthop);
	return 0;
}