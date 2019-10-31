#include "sysinclude.h"
#include<deque>
using namespace std;

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef enum {data, ack, nak} frame_kind;
typedef struct frame_head {
	frame_kind kind;
	unsigned seq, ack;
	char data[100];
};
struct frame{
	frame_head head;
	unsigned size;
};
struct FrameGo{
	frame *pfrm;
	unsigned sz;
};

deque<FrameGo> swin, waiting;
unsigned winsize = 1, counter, items;

/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType) {
    unsigned ack, num;
	FrameGo F;
	
    switch (messageType) {
    case MSG_TYPE_TIMEOUT:	// send the prepared seq
		num = ntohl(*(unsigned*)pBuffer);
		for (unsigned i = 0; i < swin.size(); i++){	// Iterate to find the frame to resend
			if (swin[i].pfrm->head.seq == num){
				SendFRAMEPacket((unsigned char*)(swin[i].pfrm), swin[i].sz);
				break;
			}
		}
        break;

    case MST_TYPE_SEND:		// Prepare a new frame
		F.pfrm = malloc(sizeof(frame));
		*F.pfrm = *(frame*)pBuffer;
		F.sz = bufferSize;
		waiting.push_back(F);
		if (swin.size() >= winsize) 		// Is there the window full?
			break;		

		F = waiting.front(); 
		waiting.pop_front();
		SendFRAMEPacket((unsigned char*)(F.pfrm), F.sz);
		swin.push_back(F);
		break;

	case MSG_TYPE_RECEIVE:	// Receive ack
		ack = ntohl((frame*)pBuffer)->head.ack);

		if (ack == swin.front().pfrm->head.seq){
			swin.pop_front();
			if (waiting.size() != 0) {
				F = waiting.front();
				waiting.pop_front();
				SendFRAMEPacket((unsigned char*)(F.pfrm), F.sz);
				swin.push_back(F);
			}
		}
		// Just ignore incorrect ack numbers...
		break;

    default: break;
    }
    return 0;
}


/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
return 0;
}


/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
return 0;
}
