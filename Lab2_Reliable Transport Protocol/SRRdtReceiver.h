#ifndef SR_RDT_RECEIVER_H
#define SR_RDT_RECEIVER_H
#include "RdtReceiver.h"
class SRRdtReceiver :public RdtReceiver
{
private:
	int NextSeqNum;					// 期待收到的下一个报文序号
	Packet AckPkt;					//确认报文
	int base;						//基序号
	int N;							//窗口大小
	Message* MsgWaiting;			//已接收并等待上传应用层的数据包
	int win_ptr;					//数组中标记，方便存取，指向的是数组中下一个未被利用的位置
	int* record;
public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:

	void receive(Packet &packet);	//接收报文，将被NetworkService调用
};

#endif
#pragma once
