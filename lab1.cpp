#include "sysinclude.h"
#include<iostream>
#include<deque>
using namespace std;

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef enum { data, ack, nak } frame_kind;
typedef struct frame_head {
	frame_kind kind;
	unsigned int seq, ack;
	unsigned char data[100];
};
struct frame {
	frame_head head;
	unsigned int size;
};

deque<frame> swin, waiting;
int next2acked = 0;

/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType) {
	unsigned ack, num;
	frame F;

	switch (messageType) {
	case MSG_TYPE_TIMEOUT:	// send the prepared seq
		num = *(unsigned*)pBuffer;
		for (unsigned i = 0; i < swin.size(); i++) {	// Iterate to find the frame to resend
			if (num == ntohl(swin[i].head.seq)) {
				SendFRAMEPacket((unsigned char*)(&swin[i]), swin[i].size);
				break;
			}
		} break;
	case MSG_TYPE_SEND:		// Prepare a new frame
		F = *(frame*)pBuffer;
		F.size = bufferSize;
		waiting.push_back(F);
		if (swin.size() >= WINDOW_SIZE_STOP_WAIT) 		// Is there the window full?
			break;
		F = waiting.front();
		waiting.pop_front();
		SendFRAMEPacket((unsigned char*)(&F), F.size);
		swin.push_back(F);
		break;
	case MSG_TYPE_RECEIVE:	// Receive ack
		ack = ntohl(((frame*)pBuffer)->head.ack);
		num = ntohl(swin.front().head.seq);
		while (ntohl(swin.front().head.seq) <= ack) {
			swin.pop_front();
			if (waiting.size() != 0) {
				F = waiting.front();
				waiting.pop_front();
				SendFRAMEPacket((unsigned char*)(&F), F.size);
				swin.push_back(F);
			}
		} break;
	default: break;
	}
	return 0;
}


/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType) {
	unsigned ack, num, tmp;
	frame F;

	switch (messageType) {
	case MSG_TYPE_TIMEOUT:	// send all timeout frames after num
		num = *(unsigned*)pBuffer;
		for (unsigned i = 0; i < swin.size(); i++) {	// Iterate to find the error frame
			SendFRAMEPacket((unsigned char*)(&swin[i]), swin[i].size);
		} break;
	case MSG_TYPE_SEND:		// Prepare a new frame
		F = *(frame*)pBuffer;
		F.size = bufferSize;
		waiting.push_back(F);
		if (swin.size() >= WINDOW_SIZE_BACK_N_FRAME) 		// Is the window full?
			break;
		F = waiting.front();
		waiting.pop_front();
		SendFRAMEPacket((unsigned char*)(&F), F.size);
		swin.push_back(F);
		break;
	case MSG_TYPE_RECEIVE:	// Receive ack
		ack = ntohl(((frame*)pBuffer)->head.ack);
		num = ntohl(swin.front().head.seq);
		while (ntohl(swin.front().head.seq) <= ack) {
			swin.pop_front();
			if (waiting.size() != 0) {
				F = waiting.front();
				waiting.pop_front();
				SendFRAMEPacket((unsigned char*)(&F), F.size);
				swin.push_back(F);
			}
		} break;
	default: break;
	}
	return 0;
}


/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType) {
	unsigned ack, num, tmp;
	frame F;

	switch (messageType) {
	case MSG_TYPE_SEND:		// Prepare a new frame
		F = *(frame*)pBuffer;
		F.size = bufferSize;
		waiting.push_back(F);
		if (swin.size() >= WINDOW_SIZE_BACK_N_FRAME) 		// Is the window full?
			break;
		F = waiting.front();
		waiting.pop_front();
		SendFRAMEPacket((unsigned char*)(&F), F.size);
		swin.push_back(F);
		break;
	case MSG_TYPE_RECEIVE:	// Receive ack or nak
		num = ntohl(((frame*)pBuffer)->head.kind);
		if (ntohl(((frame*)pBuffer)->head.kind) == 1) {
			ack = ntohl(((frame*)pBuffer)->head.ack);
			num = ntohl(swin.front().head.seq);
			while (ntohl(swin.front().head.seq) <= ack) {
				swin.pop_front();
				if (waiting.size() != 0) {
					F = waiting.front();
					waiting.pop_front();
					SendFRAMEPacket((unsigned char*)(&F), F.size);
					swin.push_back(F);
				}
			}
		}
		else {
			ack = ntohl(((frame*)pBuffer)->head.ack);
			for (unsigned i = 0; i < swin.size(); i++) {		// Find the frame to resend
				tmp = ntohl(swin.front().head.seq);
				if (ack == ntohl(swin[i].head.seq)) {
					SendFRAMEPacket((unsigned char*)(&swin[i]), swin[i].size);
					break;
				}
			}
		}
		break;
	default: break;
	}
	return 0;
}
