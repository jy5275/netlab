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

struct StoreType{
	frame *pfrm;
	unsigned sz;
};

deque<StoreType> mQue, mQue2;
bool sendWinFull = false;
int counter = 0;

/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType) {
    unsigned ack, num;
	StoreType s;
    switch (messageType) {
    case MSG_TYPE_TIMEOUT:	// send the prepared seq
		num = ntohl(*(unsigned*)pBuffer)
        s = mQue.front();
		if (num == s.pfrm->head.seq)
			SendFRAMEPacket((char*)(s.pfrm), s.sz);
        break;
    case MST_TYPE_SEND:		// Prepare a new frame
		s.pfrm = new frame;
		*s.pfrm = *(frame*)pBuffer;
		s.sz = bufferSize;
		mQue.push_back(s);
		if (!sendWinFull){	// Send all data in buffer
			s = m.mQue.front();
			SendFRAMEPacket((char*)(s.pfrm), s.sz);
			sendWinFull = true;
		}
		break;
	case MSG_TYPE_RECEIVE:	// Receive ack
		ack = ntohl((frame*)pBuffer)->head.ack);
		if (mQue.size()!=0){
			s = mQue.front();
			if (ntohl(s.pfrm->head.seq) == ack){	// ack seq number OK
				mQue.pop_front();
				s = mQue.front();
				SendFRAMEPacket((char*)s.pfrm, s.sz);	// send frames again???
			}
		} else {
			sendWinFull = false;	// data buffer is empty
		}
		break;
    default:
        break;
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
